const nfc = require('./index.js');

async function main() {
    console.log('Starting NFC reader test...');

    try {
        // Get available readers
        const readers = await nfc.getReaders();
        console.log('Available readers:', readers);

        if (readers && readers.length > 0) {
            const reader = readers[0];
            console.log('Using reader:', reader);

            // Custom APDU test
            console.log('\nTesting custom APDU command...');
            console.log('Please place an NFC card on the reader...');
            try {
                // Example: Select NDEF application
                const selectNDEF = 'D4400001';
                const response1 = await nfc.sendApdu(reader, selectNDEF);
                console.log('Select NDEF Response:', response1);

                // Example: Read NDEF data
                const readNDEF = '00B0000010';
                const response2 = await nfc.sendApdu(reader, readNDEF);
                console.log('Read NDEF Response:', response2);
            } catch (apduError) {
                console.error('APDU command error:', apduError);
            }

            // Write test
            console.log('\nTesting write functionality...');
            console.log('Please place a writable NFC tag on the reader...');
            const writeData = 'Hello NFC! ' + new Date().toISOString();
            const writeSuccess = await nfc.writeTag(reader, writeData);
            console.log('Write successful:', writeSuccess);

            // Read test
            console.log('\nTesting read functionality...');
            console.log('Please place the same or a different NFC tag on the reader...');
            try {
                const readData = await nfc.readTag(reader);
                console.log('Read successful!');
                console.log('Tag UID:', readData.uid);
                console.log('NDEF Text:', readData.ndef);
            } catch (readError) {
                console.error('Read error:', readError);
            }

            // Continuous reading
            console.log('\nStarting continuous reading...');
            nfc.startContinuousRead((data) => {
                console.log('\nTag detected!');
                console.log('Reader:', data.reader);
                console.log('UID:', data.uid);
                console.log('NDEF Text:', data.ndef);
                console.log('Raw Response:', data.response);
            });

            // Keep the process running
            process.on('SIGINT', () => {
                console.log('\nStopping NFC reader...');
                nfc.stopContinuousRead();
                process.exit();
            });

            console.log('\nWaiting for NFC tags... (Press Ctrl+C to stop)');
        } else {
            console.error('No NFC readers found!');
        }
    } catch (err) {
        console.error('Error:', err);
    }
}

main(); 