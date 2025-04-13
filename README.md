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
   ```javascript
   const nfc = require('./nfc-tron');
   ```

2. Expose the NFC functionality to the renderer process:
   ```javascript
   ipcMain.handle('get-readers', async () => {
     return await nfc.getReaders();
   });

   ipcMain.handle('start-reading', async (event, readerName) => {
     return await nfc.startReading(readerName);
   });

   ipcMain.handle('stop-reading', async () => {
     return await nfc.stopReading();
   });
   ```

3. Use in your renderer process:
   ```javascript
   // Get available readers
   const readers = await window.nfc.getReaders();

   // Start continuous reading
   await window.nfc.startReading(selectedReader);
   window.nfc.onNfcData((data) => {
     console.log('NFC Data:', data);
   });

   // Stop reading
   await window.nfc.stopReading();
   ```

## ACR122U Specific Features

The addon includes special handling for ACR122U readers:

- Automatic firmware version detection
- UID reading
- Authentication support
- Binary read/write operations
- Predefined APDU commands

## Available Commands

- `getReaders()`: List available NFC readers
- `getReaderInfo(readerName)`: Get reader information (including firmware version for ACR122U)
- `readTag(readerName)`: Read data from an NFC tag
- `writeTag(readerName, data)`: Write data to an NFC tag
- `sendApdu(readerName, command)`: Send custom APDU command
- `startReading(readerName)`: Start continuous reading
- `stopReading()`: Stop continuous reading

## ACR122U APDU Commands

The addon provides predefined APDU commands for ACR122U readers:

```javascript
const ACR122U_COMMANDS = {
  GET_FIRMWARE: 'FF00480000',
  GET_UID: 'FFCA000000',
  LOAD_AUTHENTICATION_KEYS: 'FF82000006FFFFFFFFFFFF',
  AUTHENTICATE: 'FF860000050100006000',
  READ_BINARY: 'FFB0000010',
  UPDATE_BINARY: 'FFD6000010'
};
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

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

MIT License - See LICENSE file for details