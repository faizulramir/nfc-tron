"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.sendApdu = exports.writeTag = exports.readTag = exports.getReaderInfo = exports.getReaders = exports.ACR122U_COMMANDS = void 0;
exports.startContinuousRead = startContinuousRead;
exports.stopContinuousRead = stopContinuousRead;
const util_1 = require("util");
// Import the native addon
const nfcAddon = require('../build/Release/nfcaddon');
// ACR122U specific commands
exports.ACR122U_COMMANDS = {
    GET_FIRMWARE: 'FF00480000',
    GET_UID: 'FFCA000000',
    LOAD_AUTHENTICATION_KEYS: 'FF82000006FFFFFFFFFFFF',
    AUTHENTICATE: 'FF860000050100006000',
    READ_BINARY: 'FFB0000010',
    UPDATE_BINARY: 'FFD6000010'
};
// Convert all functions to Promise-based
exports.getReaders = (0, util_1.promisify)(nfcAddon.getReaders);
exports.getReaderInfo = (0, util_1.promisify)(nfcAddon.getReaderInfo);
exports.readTag = (0, util_1.promisify)(nfcAddon.readTag);
exports.writeTag = (0, util_1.promisify)(nfcAddon.writeTag);
exports.sendApdu = (0, util_1.promisify)(nfcAddon.sendApdu);
// Keep track of continuous reading state
let isContinuousReading = false;
let continuousCallback = null;
// Start continuous reading
function startContinuousRead(callback) {
    if (isContinuousReading) {
        throw new Error('Continuous reading is already active');
    }
    isContinuousReading = true;
    continuousCallback = callback;
    // Use a polling approach for continuous reading
    const pollInterval = setInterval(async () => {
        if (!isContinuousReading) {
            clearInterval(pollInterval);
            return;
        }
        try {
            const readers = await (0, exports.getReaders)();
            if (readers.length > 0) {
                const reader = readers[0];
                const result = await (0, exports.readTag)(reader);
                if (result && continuousCallback) {
                    continuousCallback(result);
                }
            }
        }
        catch (error) {
            console.error('Error in continuous read:', error);
        }
    }, 1000); // Poll every second
    // Store the interval ID for cleanup
    continuousCallback.intervalId = pollInterval;
}
// Stop continuous reading
function stopContinuousRead() {
    if (!isContinuousReading) {
        return;
    }
    isContinuousReading = false;
    if (continuousCallback && continuousCallback.intervalId) {
        clearInterval(continuousCallback.intervalId);
    }
    continuousCallback = null;
}
//# sourceMappingURL=index.js.map