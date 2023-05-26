
const VH_SERIAL_SPEED = 115200;
const BUFFER_SIZE = 256;
const FRAME_SIZE = 12;
const FRAME_START = 0xF0;
const FRAME_END = 0xAA;

/* ================================================ */

class Frame {
    constructor() {
        this.buffer = new Uint8Array(FRAME_SIZE);
        this.reset();
    }

    get sid() {
        let dataview = new DataView(this.buffer.slice(0,4).buffer);
        return dataview.getUint32(0, false);
    }

    get high() {
        let dataview = new DataView(this.buffer.slice(4,8).buffer);
        return dataview.getUint32(0, false);
    }

    get low() {
        let dataview = new DataView(this.buffer.slice(8,12).buffer);
        return dataview.getUint32(0, false);
    }

    reset() {
        this.length = 0;
    }

    isNew() {
        return this.length == 0;
    }

    isReady() {
        return this.length == FRAME_SIZE;
    }

    free() {
        return FRAME_SIZE - this.length;
    }

    amend(data) {
        this.buffer.set(data, this.length);
        this.length += data.length;
    }
}

var log = {};

/* ================================================ */

let port;
let reader;
let isRunning;
let domTBody;
//let buffer;
//let buffer = new ArrayBuffer(BUFFER_SIZE);
let frame = new Frame();

window.addEventListener('load', e => {
    domTBody = document.getElementById('logs');
});

/* ================================================ */

async function start() {
    try {
        port = await navigator.serial.requestPort();
        if (port) {
            await port.open({ baudRate: VH_SERIAL_SPEED, bufferSize: BUFFER_SIZE });
            reader = port.readable.getReader({ mode: 'byob' });
            //reader = port.readable.getReader();
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
        //let buffer = new ArrayBuffer(BUFFER_SIZE);
        try {
            do {
                const { value, done } = await reader.read(new Uint8Array(BUFFER_SIZE));
                //const { value, done } = await reader.read();
                if (done) {
                    reader.releaseLock();
                    break;
                }
                //buffer = value.buffer;
                //console.log(value);

                //await processSome(buffer);
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
        if (frame.isReady()) {
            postFrame();
        } else {
            console.error("unsynchronized (splitted)");
        }
        while (buffer[i++] != FRAME_END && i<buffer.length);
        frame.reset();
    }
    // stuff some Frame
    while (i < buffer.length-FRAME_SIZE) {
        while (buffer[i++] != FRAME_START && i<buffer.length);
        if (i == buffer.length) {
            return;
        }
        frame.amend(buffer.slice(i,i+FRAME_SIZE));
        i+= FRAME_SIZE;
        if (frame.isReady()) {
            postFrame();
        } else {
            console.error("unsynchronized");
        }
        while (buffer[i++] != FRAME_END && i<buffer.length);
        frame.reset();
    }

    // prepare next Frame
    {
        while (buffer[i++] != FRAME_START && i<buffer.length);
        if (i == buffer.length) {
            return;
        }
        frame.amend(buffer.slice(i,buffer.length));
    }
}


async function postFrame() {
    const sid = frame.sid;
    const high = frame.high;
    const low = frame.low;
    const full = high * 0xFFFFFFFF + low;

    if (log[sid]) {
        let cache = log[sid];
        cache._count = cache._count + 1;
        
        if (full < cache.fmin) {
            cache.fmin = full;
            cache.fminc = 1;
        } else if (full > cache.fmax) {
            cache.fmax = full;
            cache.fmaxc = 1;
        } else {
            if (full == cache.fmin){
                cache.fminc = cache.fminc + 1;
            }
            if (full == cache.fmax){
                cache.fmaxc = cache.fmaxc + 1;
            }
        }
        
        if (high < cache.hmin) {
            cache.hmin = high;
            cache.hminc = 1;
        } else if (high > cache.hmax) {
            cache.hmax = high;
            cache.hmaxc = 1;
        } else {
            if (high == cache.hmin){
                cache.hminc = cache.hminc + 1;
            }
            if (high == cache.hmax){
                cache.hmaxc = cache.hmaxc + 1;
            }
        }
    } else {
        log[sid] = {
            _initial: full,
            _count: 1,
            fmin : full,
            fminc : 1,
            fmax: full,
            fmaxc: 1,
            hmin: high,
            hminc: 1,
            hmax: high,
            hmaxc: 1,
        };
    }

    const cache = log[sid];
    const hexId = sid.toString(16).toUpperCase();
    const domId = `sid_${hexId}`;

    let htmlTr = document.getElementById(domId);
    let domTr = document.createElement('tr');
    domTr.id = domId;

    domTr.appendChild(createTd(hexId));
    domTr.appendChild(createTdHex(cache._initial));
    domTr.appendChild(createTd(cache._count));
    domTr.appendChild(createTdHex(cache.fmin));
    domTr.appendChild(createTd(cache.fminc));
    domTr.appendChild(createTdHex(cache.fmax));
    domTr.appendChild(createTd(cache.fmaxc));
    domTr.appendChild(createTdHex(cache.hmin));
    domTr.appendChild(createTd(cache.hminc));
    domTr.appendChild(createTdHex(cache.hmax));
    domTr.appendChild(createTd(cache.hmaxc));

    if (cache.fminc < cache.fmaxc) {
        domTr.classList.add('fmax');
    } else if (cache.fminc > cache.fmaxc) {
        domTr.classList.add('fmin');
    } else {
        domTr.classList.add('fequal');
    }

    if (cache.hminc < cache.hmaxc) {
        domTr.classList.add('hmax');
    } else if (cache.hminc > cache.hmaxc) {
        domTr.classList.add('hmin');
    } else {
        domTr.classList.add('hequal');
    }
    
    if (htmlTr) {
        domTBody.replaceChild(domTr, htmlTr);
    } else {
        domTBody.appendChild(domTr);
    }
}


function createTd(textNode) {
    let td = document.createElement('td');
    const raw = document.createTextNode(textNode);

    td.appendChild(raw);

    return td;
}


function createTdHex(textNode) {
    let td = document.createElement('td');
    const raw = document.createTextNode(textNode);
    const code = document.createElement('code');
    const hex = document.createTextNode(textNode.toString(16).toUpperCase());

    code.appendChild(hex);

    td.appendChild(raw);
    td.appendChild(code);

    return td;
}


function toCsvUri() {
    let csvContent = "data:text/csv;charset=utf-8,";

    for (const [key, values] of Object.entries(log)) {
        csvContent+= key + ',' + Object.values(values).join(',') + '\n';
    }
    
    return encodeURI(csvContent);
}


// setInterval(run, 100);
//run();