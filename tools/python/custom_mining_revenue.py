import csv
import sys

NUMBER_OF_COMPUTORS = 676
UINT64_SIZE = 8

def computeNewScore(oldScore, customScore):
    new_score = oldScore * customScore
    return new_score

class RevenueScore:
    def __init__(self):
        self.old_final_score = [0] * NUMBER_OF_COMPUTORS
        self.custom_mining_score = [0] * NUMBER_OF_COMPUTORS

def bytes_to_uint64(byte_data):
    """Convert 8 bytes (little-endian) to an unsigned 64-bit integer."""
    value = 0
    for i in range(UINT64_SIZE):
        value |= byte_data[i] << (i * 8)
    return value

def dump_custom_mining_share_to_csv(input_file, output_file):
    custom_mining_rev = RevenueScore()

    try:
        with open(input_file, "rb") as file:
            # Read old final scores
            for i in range(NUMBER_OF_COMPUTORS):
                byte_data = file.read(UINT64_SIZE)
                if len(byte_data) != UINT64_SIZE:
                    raise ValueError("Unexpected end of file.")
                custom_mining_rev.old_final_score[i] = bytes_to_uint64(byte_data)

            # Read custom mining scores
            for i in range(NUMBER_OF_COMPUTORS):
                byte_data = file.read(UINT64_SIZE)
                if len(byte_data) != UINT64_SIZE:
                    raise ValueError("Unexpected end of file.")
                custom_mining_rev.custom_mining_score[i] = bytes_to_uint64(byte_data)

    except FileNotFoundError:
        print(f"Error: Cannot open file '{input_file}'")
        exit(1)

    try:
        with open(output_file, "w", newline="") as file:
            writer = csv.writer(file)
            
            # Write header
            writer.writerow(["Index", "OldFinalScore", "CustomMiningScore", "NewScore"])

            # Write data
            for i in range(NUMBER_OF_COMPUTORS):
                old_score = custom_mining_rev.old_final_score[i]
                custom_score = custom_mining_rev.custom_mining_score[i]
                new_score = computeNewScore(old_score, custom_score)
                writer.writerow([i, old_score, custom_score, new_score])

        print(f"CSV file '{output_file}' written successfully.")

    except Exception as e:
        print(f"Error writing file '{output_file}': {e}")
        exit(1)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python/python3 custom_mining_revenue.py <input_file> <output_file>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    dump_custom_mining_share_to_csv(input_file, output_file)