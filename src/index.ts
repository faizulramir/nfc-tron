import { promisify } from 'util';

interface NfcAddon {
    getReaders(callback: (error: Error | null, readers: string[]) => void): void;
    getReaderInfo(reader: string, callback: (error: Error | null, info: any) => void): void;
    readTag(reader: string, callback: (error: Error | null, result: any) => void): void;
    writeTag(reader: string, data: string, callback: (error: Error | null, result: any) => void): void;
    sendApdu(reader: string, command: string, callback: (error: Error | null, result: any) => void): void;
}

interface TagResult {
    uid: string;
    data?: string;
    [key: string]: any;
}

interface ReaderInfo {
    name: string;
    state: string;
    [key: string]: any;
}

// Import the native addon
const nfcAddon: NfcAddon = require('../build/Release/nfcaddon');

// ACR122U specific commands
export const ACR122U_COMMANDS = {
    GET_FIRMWARE: 'FF00480000',
    GET_UID: 'FFCA000000',
    LOAD_AUTHENTICATION_KEYS: 'FF82000006FFFFFFFFFFFF',
    AUTHENTICATE: 'FF860000050100006000',
    READ_BINARY: 'FFB0000010',
    UPDATE_BINARY: 'FFD6000010'
} as const;

// Convert all functions to Promise-based
export const getReaders = promisify(nfcAddon.getReaders) as () => Promise<string[]>;
export const getReaderInfo = promisify(nfcAddon.getReaderInfo) as (reader: string) => Promise<ReaderInfo>;
export const readTag = promisify(nfcAddon.readTag) as (reader: string) => Promise<TagResult>;
export const writeTag = promisify(nfcAddon.writeTag) as (reader: string, data: string) => Promise<any>;
export const sendApdu = promisify(nfcAddon.sendApdu) as (reader: string, command: string) => Promise<any>;

// Keep track of continuous reading state
let isContinuousReading = false;
let continuousCallback: ((result: TagResult) => void) | null = null;

// Start continuous reading
export function startContinuousRead(callback: (result: TagResult) => void): void {
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
            const readers = await getReaders();
            if (readers.length > 0) {
                const reader = readers[0];
                const result = await readTag(reader);
                if (result && continuousCallback) {
                    continuousCallback(result);
                }
            }
        } catch (error) {
            console.error('Error in continuous read:', error);
        }
    }, 1000); // Poll every second

    // Store the interval ID for cleanup
    (continuousCallback as any).intervalId = pollInterval;
}

// Stop continuous reading
export function stopContinuousRead(): void {
    if (!isContinuousReading) {
        return;
    }

    isContinuousReading = false;
    if (continuousCallback && (continuousCallback as any).intervalId) {
        clearInterval((continuousCallback as any).intervalId);
    }
    continuousCallback = null;
} 