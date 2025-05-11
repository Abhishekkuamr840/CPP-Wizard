# ABX Mock Exchange Client

A C++17 client implementation for the ABX Mock Exchange system. This client connects to the ABX server, streams market data packets, handles missing sequences, and outputs the data in JSON format.

## Features

- TCP-based client-server communication
- Automatic handling of missing sequence numbers
- Big-endian packet parsing
- JSON output format
- Automated setup and execution script

## Requirements

- C++17 compatible compiler (g++ 7.0 or later)
- Node.js ≥ 16.17.0
- Git
- Windows OS (for networking headers)

## Project Structure

```
.
├── abx_client.cpp    # Main client implementation
├── run_abx.sh        # Automation script
├── packets.json      # Output file (generated)
└── README.md         # This file
```

## Packet Format

The client handles 17-byte packets with the following structure:
- Symbol (4 bytes)
- Buy/Sell indicator (1 byte)
- Quantity (4 bytes, big-endian)
- Price (4 bytes, big-endian)
- Sequence number (4 bytes, big-endian)

## JSON Output Format

```json
[
  {
    "symbol": "MSFT",
    "buysellindicator": "B",
    "quantity": 100,
    "price": 250,
    "packetSequence": 42
  }
]
```

## Setup and Usage

1. Clone the repository:
   ```bash
   git clone https://github.com/your-username/abx-client.git
   cd abx-client
   ```

2. Make the automation script executable:
   ```bash
   chmod +x run_abx.sh
   ```

3. Run the automation script:
   ```bash
   ./run_abx.sh
   ```

The script will:
- Check Node.js version
- Start the ABX server
- Compile the client
- Prompt for server host (default: 127.0.0.1)
- Run the client
- Generate packets.json

## Manual Compilation

If you prefer to compile manually:

```bash
g++ -std=c++17 -o abx_client abx_client.cpp
./abx_client 127.0.0.1 3000
```

## Error Handling

The client includes comprehensive error handling for:
- Network connection issues
- Invalid packet formats
- Missing sequences
- File I/O errors

## Development

### Building from Source

1. Ensure you have the required dependencies:
   ```bash
   # Check g++ version
   g++ --version
   
   # Check Node.js version
   node --version
   ```

2. Compile the client:
   ```bash
   g++ -std=c++17 -o abx_client abx_client.cpp
   ```

### Testing

The client automatically verifies:
- Complete sequence coverage
- Valid JSON output
- Proper packet parsing

## License

[Your chosen license]

## Contributing

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request 