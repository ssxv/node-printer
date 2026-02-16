// Network-based printing that bypasses OS print spoolers
// For POS/industrial printers using raw TCP connections

import * as net from 'net';
import { PrintSocketOptions } from './types';
import { PrinterError, PrinterErrorCode } from './errors';

/**
 * Validate print socket options
 */
function validatePrintSocketOptions(options: PrintSocketOptions): void {
  if (!options.host || typeof options.host !== 'string') {
    throw new PrinterError('Host address is required and must be a string', 'INVALID_ARGUMENTS');
  }

  if (!options.data || !Buffer.isBuffer(options.data)) {
    throw new PrinterError('Data is required and must be a Buffer', 'INVALID_ARGUMENTS');
  }

  if (options.port !== undefined && (typeof options.port !== 'number' || options.port <= 0 || options.port > 65535)) {
    throw new PrinterError('Port must be a valid number between 1 and 65535', 'INVALID_ARGUMENTS');
  }

  if (options.timeout !== undefined && (typeof options.timeout !== 'number' || options.timeout <= 0)) {
    throw new PrinterError('Timeout must be a positive number in milliseconds', 'INVALID_ARGUMENTS');
  }

  if (options.retries !== undefined && (typeof options.retries !== 'number' || options.retries < 0)) {
    throw new PrinterError('Retries must be a non-negative number', 'INVALID_ARGUMENTS');
  }
}

/**
 * Attempt to connect and send data to a network printer
 */
function attemptSocketPrint(options: PrintSocketOptions): Promise<void> {
  return new Promise((resolve, reject) => {
    const port = options.port || 9100;
    const host = options.host;
    const data = options.data;

    // If timeout is provided, wait for connection stability. If not, fire and forget
    const useTimeout = options.timeout !== undefined;
    const connectionTimeout = options.timeout || 5000; // Default 5s for connection if no timeout

    let isResolved = false;

    const connection = net.connect(
      {
        host,
        port,
        timeout: connectionTimeout
      },
      () => {
        // Connection established, send data
        connection.write(data, writeError => {
          if (writeError && !isResolved) {
            isResolved = true;
            connection.destroy();
            reject(
              new PrinterError(
                `Failed to write data to ${host}:${port}: ${writeError.message}`,
                'NETWORK_ERROR',
                writeError
              )
            );
            return;
          }

          // If not using timeout, complete immediately after write
          if (!useTimeout && !isResolved) {
            isResolved = true;
            connection.destroy();
            resolve();
          }
        });
      }
    );

    // Handle data received from printer (only relevant if timeout was provided)
    connection.on('data', (data: Buffer) => {
      if (useTimeout && !isResolved) {
        isResolved = true;
        connection.destroy();
        resolve();
      }
    });

    // Handle connection errors
    connection.on('error', (error: Error) => {
      if (!isResolved) {
        isResolved = true;
        connection.destroy();

        // Map common network errors to more descriptive messages
        let errorMessage = `Network error connecting to ${host}:${port}`;
        let errorCode: PrinterErrorCode = 'NETWORK_ERROR';

        if ((error as any).code === 'ECONNREFUSED') {
          errorMessage = `Connection refused to printer at ${host}:${port}. Printer may be offline or port blocked.`;
          errorCode = 'CONNECTION_REFUSED';
        } else if ((error as any).code === 'EHOSTUNREACH') {
          errorMessage = `Host unreachable: ${host}. Check network connectivity.`;
          errorCode = 'HOST_UNREACHABLE';
        } else if ((error as any).code === 'ENOTFOUND') {
          errorMessage = `Host not found: ${host}. Check hostname/IP address.`;
          errorCode = 'HOST_NOT_FOUND';
        } else {
          errorMessage += `: ${error.message}`;
        }

        reject(new PrinterError(errorMessage, errorCode, error));
      }
    });

    // Handle timeout
    connection.on('timeout', () => {
      if (!isResolved) {
        isResolved = true;
        connection.destroy();
        reject(new PrinterError(`Connection timeout to ${host}:${port} after ${connectionTimeout}ms`, 'TIMEOUT'));
      }
    });

    // Handle connection close
    connection.on('close', () => {
      // If we're using timeout and connection closed without any data exchange
      if (useTimeout && !isResolved) {
        isResolved = true;
        reject(new PrinterError(`Connection closed by ${host}:${port} unexpectedly`, 'NETWORK_ERROR'));
      }
    });
  });
}

export const network = {
  /**
   * Print raw data directly to a network printer via TCP socket
   *
   * WARNING: This bypasses the OS print spooler entirely.
   * - No job tracking or status feedback
   * - No driver support or capability negotiation
   * - Success only means TCP data was sent, not that printing completed
   *
   * If timeout is provided: waits for printer to accept data within that time.
   * If timeout is omitted: fire-and-forget mode (no waiting).
   *
   * Intended for POS/industrial printers with ESC/POS, ZPL, or similar raw command languages.
   * For enterprise printing with job tracking, use jobs.printFile() or jobs.printRaw() instead.
   */
  async printSocket(options: PrintSocketOptions): Promise<void> {
    try {
      validatePrintSocketOptions(options);

      const maxRetries = options.retries || 0;
      let lastError: Error;

      for (let attempt = 0; attempt <= maxRetries; attempt++) {
        try {
          return await attemptSocketPrint(options);
        } catch (error) {
          lastError = error instanceof Error ? error : new Error(String(error));

          // If this is the last attempt, throw the error
          if (attempt === maxRetries) {
            break;
          }

          // Brief delay before retry to avoid hammering the printer
          if (attempt < maxRetries) {
            await new Promise(resolve => setTimeout(resolve, 1000 * (attempt + 1)));
          }
        }
      }

      // If we get here, all retries failed
      const totalAttempts = maxRetries + 1;
      throw new PrinterError(
        `Failed to print to ${options.host}:${options.port || 9100} after ${totalAttempts} attempt(s). Last error: ${lastError!.message}`,
        'NETWORK_ERROR',
        lastError!
      );
    } catch (error) {
      if (error instanceof PrinterError) {
        throw error;
      }
      throw new PrinterError(`Unexpected error in printSocket: ${error}`, 'UNKNOWN', error);
    }
  }
};
