const nfcAddon = require('./build/Release/nfcaddon');

// Helper function to convert callback-based functions to Promise-based
function promisify(fn) {
  return (...args) => {
    return new Promise((resolve, reject) => {
      try {
        fn(...args, (err, result) => {
          if (err) {
            if (err instanceof Error) {
              reject(err);
            } else {
              reject(new Error(err));
            }
          } else {
            resolve(result);
          }
        });
      } catch (error) {
        reject(error);
      }
    });
  };
}

// Convert all functions to Promise-based
const getReaders = promisify(nfcAddon.getReaders);
const getReaderInfo = promisify(nfcAddon.getReaderInfo);
const readTag = promisify(nfcAddon.readTag);
const writeTag = promisify(nfcAddon.writeTag);
const sendApdu = promisify(nfcAddon.sendApdu);

// ACR122U specific commands
const ACR122U_COMMANDS = {
  GET_FIRMWARE: 'FF00480000',
  GET_UID: 'FFCA000000',
  LOAD_AUTHENTICATION_KEYS: 'FF82000006FFFFFFFFFFFF',
  AUTHENTICATE: 'FF860000050100006000',
  READ_BINARY: 'FFB0000010',
  UPDATE_BINARY: 'FFD6000010'
};

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

module.exports = {
  getReaders,
  getReaderInfo,
  readTag,
  writeTag,
  sendApdu,
  startContinuousRead,
  stopContinuousRead,
  ACR122U_COMMANDS
};
