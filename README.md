# NFC-Tron

A native addon for Electron applications to interact with NFC readers, specifically optimized for ACR122U readers.

## Status: 🚧 Ongoing Development

This project is currently under active development. Features are being added and improved regularly.

## Prerequisites

- Windows 10 or later
- Node.js (v14 or later)
- Python 3.x (for node-gyp)
- Visual Studio Build Tools (for C++ compilation)
- ACR122U NFC Reader or compatible device
- Smart Card service must be running on Windows

## Installation

1. Install the required build tools:
   ```bash
   npm install -g node-gyp
   ```

2. Install Visual Studio Build Tools with C++ workload

3. Clone the repository:
   ```bash
   git clone https://github.com/faizulramir/nfc-tron.git
   cd nfc-tron
   ```

4. Install dependencies:
   ```bash
   npm install
   ```

5. Build the native addon:
   ```bash
   node-gyp rebuild
   ```

## TypeScript Support

The project now includes full TypeScript support:

1. TypeScript configuration is available in `tsconfig.json`
2. Source files are in the `src` directory
3. Compiled JavaScript files are in the `dist` directory
4. Type definitions are automatically generated

To build TypeScript files:
```bash
npm run build
```

To watch for changes during development:
```bash
npm run watch
```

## Windows Smart Card Service Setup

1. Open PowerShell as Administrator
2. Start the Smart Card service:
   ```powershell
   Start-Service -Name SCardSvr
   ```
3. Verify the service is running:
   ```powershell
   Get-Service -Name SCardSvr
   ```

## Usage in Electron Application

1. Import the addon in your main process:
   ```typescript
   import { getReaders, readTag, writeTag, sendApdu, startContinuousRead, stopContinuousRead, ACR122U_COMMANDS } from './nfc-tron';
   ```

2. Expose the NFC functionality to the renderer process:
   ```typescript
   ipcMain.handle('get-readers', async () => {
     return await getReaders();
   });

   ipcMain.handle('start-reading', async (event, readerName: string) => {
     return await startContinuousRead((data) => {
       // Handle NFC data
       console.log('NFC Data:', data);
     });
   });

   ipcMain.handle('stop-reading', async () => {
     return await stopContinuousRead();
   });
   ```

3. Use in your renderer process:
   ```typescript
   // Get available readers
   const readers: string[] = await window.nfc.getReaders();

   // Start continuous reading
   await window.nfc.startContinuousRead((data) => {
     console.log('NFC Data:', data);
   });

   // Stop reading
   await window.nfc.stopContinuousRead();
   ```

## ACR122U Specific Features

The addon includes special handling for ACR122U readers:

- Automatic firmware version detection
- UID reading
- Authentication support
- Binary read/write operations
- Predefined APDU commands

## Available Commands

- `getReaders(): Promise<string[]>`: List available NFC readers
- `getReaderInfo(readerName: string): Promise<ReaderInfo>`: Get reader information
- `readTag(readerName: string): Promise<TagResult>`: Read data from an NFC tag
- `writeTag(readerName: string, data: string): Promise<any>`: Write data to an NFC tag
- `sendApdu(readerName: string, command: string): Promise<any>`: Send custom APDU command
- `startContinuousRead(callback: (result: TagResult) => void): void`: Start continuous reading
- `stopContinuousRead(): void`: Stop continuous reading

## ACR122U APDU Commands

The addon provides predefined APDU commands for ACR122U readers:

```typescript
const ACR122U_COMMANDS = {
  GET_FIRMWARE: 'FF00480000',
  GET_UID: 'FFCA000000',
  LOAD_AUTHENTICATION_KEYS: 'FF82000006FFFFFFFFFFFF',
  AUTHENTICATE: 'FF860000050100006000',
  READ_BINARY: 'FFB0000010',
  UPDATE_BINARY: 'FFD6000010'
} as const;
```

## Troubleshooting

1. If you encounter permission errors:
   - Ensure you're running the application with administrator privileges
   - Verify the Smart Card service is running
   - Check if the reader is properly connected and recognized by Windows

2. If the addon fails to build:
   - Verify all prerequisites are installed
   - Check Visual Studio Build Tools installation
   - Ensure Python 3.x is in your PATH

3. If readers are not detected:
   - Restart the Smart Card service
   - Reconnect the NFC reader
   - Check device manager for any driver issues

4. If TypeScript compilation fails:
   - Ensure all dependencies are installed
   - Check tsconfig.json for correct configuration
   - Verify source files are in the src directory

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

MIT License - See LICENSE file for details