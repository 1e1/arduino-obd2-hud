
const VH_SERIAL_SPEED = 115200;
const BUFFER_SIZE = 256;
const FRAME_SIZE = 15;
const FRAME_START = 0xF0;
const FRAME_END = 0xAA;

const CLASSNAME_ISMARKED = 'isMarked';
const LABEL_8BITS = 'b8';
const LABEL_16BITS = 'b16';

/* ================================================ */

class Tty {
    // std: Type{1}Identifier{3}Length{1}Data{0,8}Timestamp{4}?\r
    // xtd: Type{1}Identifier{8}Length{1}Data{0,8}Timestamp{4}?\r
}

class Frame {
    static get TYPE_SINGLEFRAME() {
        return 0;
    };

    static get TYPE_FIRSTFRAME() {
        return 1;
    };
    
    static get TYPE_CONSECUTIVEFRAME() {
        return 2;
    };

    static get TYPE_FLOWCONTROL() {
        return 3;
    };

    static counter = 0;

    constructor() {
        this.buffer = new Uint8Array(FRAME_SIZE);
        this.reset();
    }

    get index() {
        return this.counter;
    }

    get pgn() {
        let dataview = new DataView(this.buffer.slice(0,4).buffer);
        return dataview.getUint32(0, false);
    }

    get isRemoteRequest() {
        return this.buffer[4];
    }

    get isExtendedFrame() {
        return this.buffer[5];
    }

    get packetLength() {
        return this.buffer[6];
    }

    get type() {
        return this.buffer[7] >> 4;
    }

    getDataLength() {
        switch (this.type) {
            case Frame.TYPE_SINGLEFRAME:
                return (this.buffer[7] & 0xF);
            case Frame.TYPE_FIRSTFRAME:
                return ((this.buffer[7] & 0xF) << 8) + this.buffer[8];
        }

        return -1;
    }

    getSeqNum() {
        if (this.type == Frame.TYPE_CONSECUTIVEFRAME) {
            return this.buffer[7];
        }

        return -1;
    }

    getData() {
        return this.buffer.slice(8);
    }

    reset() {
        this.length = 0;
        Frame.counter = Frame.counter + 1;
        this.counter = Frame.counter;
    }

    isNew() {
        return 0 == this.length;
    }

    isFull() {
        return FRAME_SIZE == this.length;
    }

    free() {
        return FRAME_SIZE - this.length;
    }

    amend(data) {
        this.buffer.set(data, this.length);
        this.length += data.length;
    }
}

class Message {
    constructor(pgn, size, meta) {
        this.pgn = pgn;
        this.src = pgn & 0xFF;
        this.data = new Uint8Array(size);
        this.meta = meta;
        this.length = 0;
    }

    isFull() {
        return this.data.length == this.length;
    }

    free() {
        return this.data.length - this.length;
    }

    amend(data) {
        this.data.set(data, this.length);
        this.length += data.length;
    }
}

class MessageObd2 extends Message {
    get sid() {
        return this.data[0];
    }

    get pid8() {
        return this.data[1];
    }

    get pid16() {
        let dataview = new DataView(this.data.slice(1,3));
        return dataview.getUint16(0, false);
    }

    getPid(isPid16) {
        return isPid16 ? this.pid16 : this.pid8;
    }

    getData(isPid16) {
        const offset = 2 + (true==isPid16);
        return this.buffer.slice(offset);
    }
}

var log = {
    messages: [],
};
/**
 * log = {
 *  cerr: 123,
 *  count: 12345,
 *  frame.pgn: {
 *      LABEL_8BITS: {
 *          frame.pid8: [
 *              *{ order: frame.counter, isMarked: isMarked, sid: frame.sid, length: frame.length, data: frame.data(false), }
 *          ],
 *      },
 *      LABEL_16BITS: {
 *          frame.pid16: [
 *              *{ order: frame.counter, isMarked: isMarked, sid: frame.sid, length: frame.length, data: frame.data(true), }
 *          ],
 *      },
 *  }
 * }
 */

/* ================================================ */

let port;
let reader;
let isRunning;
let domBody;
let domTBody;
let domOutputErrorCounter;
let errorCounter = 0;
let frame = new Frame();
let message;
let isMarked;

window.addEventListener('load', e => {
    let wordsToArray = str => str.split(',').map(i=>i.trim().toUpperCase());

    domBody = document.querySelector('body');
    domTBody = document.getElementById('trace');
    domOutputErrorCounter = document.getElementById('cerr');
    isMarked = document.getElementById('marker').checked;
});

/* ================================================ */

async function start() {
    try {
        port = await navigator.serial.requestPort();
        if (port) {
            await port.open({ baudRate: VH_SERIAL_SPEED, bufferSize: BUFFER_SIZE });
            reader = port.readable.getReader({ mode: 'byob' });
            isRunning = true;
            run();
        }
    } catch (e) {
        console.error(e);
    }
}


async function terminate() {
    isRunning = false;
    await port.close();
}


async function run() {
    while (isRunning && port.readable) {
        try {
            do {
                const { value, done } = await reader.read(new Uint8Array(BUFFER_SIZE));
                if (done) {
                    reader.releaseLock();
                    break;
                }
                
                await processSome(value);
            } while(isRunning);
        } catch (error) {
            isRunning = false;
            console.error(error);
        }
    }
}


async function processSome(buffer) {
    let i = 0;
    // finish previous Frame
    if (!frame.isNew()) {
        i = frame.free();
        frame.amend(buffer.slice(0,i));
        if (frame.isFull()) {
            postFrame();
        } else {
            updateErrorCounter('unsynchronized (splitted)');
        }
        while (i<buffer.length && buffer[i++] != FRAME_END);
        frame.reset();
    }
    // stuff some Frame
    while (i < buffer.length-(FRAME_SIZE+2-1)) {
        while (i<buffer.length && buffer[i++] != FRAME_START);
        if (i == buffer.length) {
            return;
        }
        frame.amend(buffer.slice(i,i+FRAME_SIZE));
        i+= FRAME_SIZE;
        if (frame.isFull()) {
            postFrame();
        } else {
            console.log(frame.length);
            updateErrorCounter('unsynchronized');
        }
        while (i<buffer.length && buffer[i++] != FRAME_END);
        frame.reset();
    }

    // prepare next Frame
    {
        while (i<buffer.length && buffer[i++] != FRAME_START);
        if (i == buffer.length) {
            return;
        }
        frame.amend(buffer.slice(i,buffer.length));
    }
}


function updateErrorCounter(reason) {
    console.error(reason);
    domOutputErrorCounter.value = ++errorCounter;
}


async function postFrame() {
    switch (frame.type) {
        case Frame.TYPE_SINGLEFRAME:
            {
                const meta = {
                    isSingle: 1,
                    isRemoteRequest: frame.isRemoteRequest,
                    isExtendedFrame: frame.isExtendedFrame,
                    isMarked: isMarked,
                    index: frame.index,
                };

                message = new Message(frame.pgn, frame.getDataLength(), meta);
                message.amend(frame.getData().slice(0, message.free()));
            }
            if (message.isFull()) {
                log.messages.push(message);
                drawTrMessage();
            } else {
                // throw error
            }
            break;
        case Frame.TYPE_FIRSTFRAME: 
            {
                const meta = {
                    isSingle: 0,
                    isRemoteRequest: frame.isRemoteRequest,
                    isExtendedFrame: frame.isExtendedFrame,
                    isMarked: isMarked,
                    index: frame.index,
                };

                message = new Message(frame.pgn, frame.getDataLength(), meta);
            }
        case Frame.TYPE_CONSECUTIVEFRAME:
            if (message.free() < frame.getDataLength()) {
                console.error('frame => message too small', frame, message);
            }
            message.amend(frame.getData().slice(0, message.free()));
    
            if (message.isFull()) {
                log.messages.push(message);
                drawTrMessage();
            }
            break;
    }
}


function createTd(textNode) {
    let td = document.createElement('td');
    const raw = document.createTextNode(textNode);

    td.appendChild(raw);

    return td;
}


function createTdCode(...textNodes) {
    let td = document.createElement('td');

    textNodes.forEach((item, index) => {
        const code = document.createElement('code');
        const txt = document.createTextNode(String(item).toUpperCase());

        code.append(txt);
        td.appendChild(code);
    
    });

    return td;
}


async function drawTrMessage() {
    let data, bin, dec, hex;

    if (message.length == 0) {
        bin = '';
        dec = '';
        hex = '';
    } else {
        const arr = Array.from(message.data);
        bin = arr.map(e=>('0000000'+e.toString(2)).slice(-8)).join(' ');
        dec = arr.map(e=>('00'+e.toString(10)).slice(-3)).join(' ')
        hex = arr.map(e=>('0'+e.toString(16)).slice(-2)).join(' ');
    }

    let domTr = document.createElement('tr');
    //domTr.classList.add('');
    if (message.meta.isMarked) {
        domTr.classList.add(CLASSNAME_ISMARKED);
    }
    domTr.dataset.groupId = message.pgn;
    domTr.dataset.serviceId = message.data[0] || 'void';
    domTr.dataset.parameterId = message.data[1] || 'void';

    domTr.appendChild(createTd(String(message.meta.index)));
    //domTr.appendChild(createTdCode(message.pgn.toString(16), message.pgn));
    domTr.appendChild(createTdCode(message.src.toString(16), message.src));
    //domTr.appendChild(createTd(String(message.meta.isSingle)));
    //domTr.appendChild(createTd(String(message.meta.isRemoteRequest)));
    //domTr.appendChild(createTd(String(message.meta.isExtendedFrame)));
    domTr.appendChild(createTdCode(hex.substring(0,2), dec.substring(0,3)));
    domTr.appendChild(createTdCode(hex.substring(3,5), dec.substring(4,7)));
    domTr.appendChild(createTd(String(message.length)));
    domTr.appendChild(createTdCode(hex));
    domTr.appendChild(createTdCode(dec));
    domTr.appendChild(createTdCode(bin));

    //const isScrolledToBottom = domBody.scrollHeight - domBody.clientHeight <= domBody.scrollTop + 1;

    domTBody.appendChild(domTr);
    /*
    if (isScrolledToBottom) {
        domBody.scrollTop = domBody.scrollHeight - domBody.clientHeight;
    }
    */
}


async function removeGroupId(groupId) {
    document.querySelectorAll('[data-group-id="'+groupId+'"]').forEach(i => {
        i.parentNode.removeChild(i);
    });
}


function toCsvUri() {
    let csvContent = 'data:text/csv;charset=utf-8,index,isMarked,pgn,source,isSingle,isRemoteRequest,isExtendedFrame,length,hex,dec,bin\n';

    log.messages.forEach(msg => {
        const arr = Array.from(msg.data);
        const bin = arr.map(e=>('0000000'+e.toString(2)).slice(-8)).join(' ');
        const dec = arr.map(e=>('00'+e.toString(10)).slice(-3)).join(' ')
        const hex = arr.map(e=>('0'+e.toString(16)).slice(-2)).join(' ');

        csvContent+= [msg.meta.index, msg.meta.isMarked, msg.pgn, msg.src, msg.meta.isSingle, msg.meta.isRemoteRequest, msg.meta.isExtendedFrame, msg.length, hex, dec, bin].join(',');
        csvContent+= '\n';
    });
    /*
    for (const pgn of Object.keys(log)) {
        const pgnint = parseInt(pgn);
        { // 8-bits
            const label8bits = pgnWithPid8.includes(pgnint) ? 'is8bits' : 'if8bits'; 
            for (const [pid, packets] of Object.entries(log[pgn][LABEL_8BITS])) {
                packets.forEach(packet => {
                    csvContent+= [pgn, label8bits, packet.sid, packet.order, pid, packet.isMarked, packet.length, packet.dec].join(',');
                    csvContent+= '\n';
                });
            }
        }
        { // 16-bits
            const label16bits = pgnWithPid16.includes(pgnint) ? 'is16bits' : 'if16bits'; 
            for (const [pid, packets] of Object.entries(log[pgn][LABEL_16BITS])) {
                packets.forEach(packet => {
                    csvContent+= [pgn, label16bits, packet.sid, packet.order, pid, packet.isMarked, packet.length, packet.dec].join(',');
                    csvContent+= '\n';
                });
            }
        }
    }
    */
    
    return encodeURI(csvContent);
}


function toCandumpUri() {
    let dumpContent = 'data:text/plain;charset=utf-8\n';

    log.messages.forEach(msg => {
        const pgn = ('00'+msg.pgn.toString(16)).slice(-3);
        const hex = Array.from(msg.data).map(e=>('0'+e.toString(16)).slice(-2)).join('');

        dumpContent+= `(${msg.meta.index} vcan0 ${pgn}#${hex})\n`;
    });
    
    return encodeURI(dumpContent);
}
