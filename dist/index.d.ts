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
export declare const ACR122U_COMMANDS: {
    readonly GET_FIRMWARE: "FF00480000";
    readonly GET_UID: "FFCA000000";
    readonly LOAD_AUTHENTICATION_KEYS: "FF82000006FFFFFFFFFFFF";
    readonly AUTHENTICATE: "FF860000050100006000";
    readonly READ_BINARY: "FFB0000010";
    readonly UPDATE_BINARY: "FFD6000010";
};
export declare const getReaders: () => Promise<string[]>;
export declare const getReaderInfo: (reader: string) => Promise<ReaderInfo>;
export declare const readTag: (reader: string) => Promise<TagResult>;
export declare const writeTag: (reader: string, data: string) => Promise<any>;
export declare const sendApdu: (reader: string, command: string) => Promise<any>;
export declare function startContinuousRead(callback: (result: TagResult) => void): void;
export declare function stopContinuousRead(): void;
export {};
