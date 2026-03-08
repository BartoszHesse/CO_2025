import subprocess
import re
import itertools
import time

# --- CONFIGURATION ---
EXECUTABLE = "./sa_tester.exe"  
DATA_FILE = "LANL-CM5-1994-3.1-cln.swf"
OUTPUT_FILE = "results.txt"

# Parameters to test
Ns = [1000, 5000]
alphas = [0.90, 0.95, 0.98, 0.99, 0.995]
t_ends = [1e-3, 1e-6, 1e-9, 1e-12, 1e-15]
swap_probs = [0.25, 0.375, 0.50, 0.625, 0.75]

# --- PARSING FUNCTION ---
def parse_makespan(stderr_output):
    """Finds the line '[SA] Final best makespan = X' and returns X."""
    match = re.search(r"Final best makespan = (\d+)", stderr_output)
    if match:
        return int(match.group(1))
    return None

# --- MAIN LOOP ---
def main():
    # Compute the number of combinations for progress tracking
    total_runs = len(Ns) * len(alphas) * len(t_ends) * len(swap_probs)
    current_run = 0

    print(f"Starting experiment. Total number of runs: {total_runs}")
    print(f"Results will be saved to: {OUTPUT_FILE}")

    with open(OUTPUT_FILE, "w") as f:
        # CSV/TXT file header
        f.write("N;Alpha;T_END;Swap_Prob;Makespan;Time_Taken_s\n")
        
        # itertools.product generates all combinations (nested loops)
        for N, alpha, t_end, swap_prob in itertools.product(Ns, alphas, t_ends, swap_probs):
            current_run += 1
            
            # Command formatting
            # ./scheduler file.swf N T_END ALPHA SWAP_PROB
            cmd = [
                EXECUTABLE,
                DATA_FILE,
                str(N),
                str(t_end),
                str(alpha),
                str(swap_prob)
            ]

            print(f"[{current_run}/{total_runs}] Running: N={N}, Alpha={alpha}, T_END={t_end}, Swap={swap_prob} ... ", end="", flush=True)
            
            start_time = time.time()
            try:
                # Run the C++ process
                # capture_output=True captures what the program prints (stderr and stdout)
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=310) # 310s timeout for safety
                
                duration = time.time() - start_time
                
                # The C++ program writes logs to cerr (stderr), so we parse the result there
                makespan = parse_makespan(result.stderr)
                
                if makespan is not None:
                    print(f"OK (Makespan: {makespan})")
                    # Write to file
                    f.write(f"{N};{alpha};{t_end};{swap_prob};{makespan};{duration:.2f}\n")
                    f.flush() # Force write to disk
                else:
                    print("ERROR (Result not found in program output)")
                    f.write(f"{N};{alpha};{t_end};{swap_prob};ERROR;{duration:.2f}\n")

            except subprocess.TimeoutExpired:
                print("TIMEOUT")
                f.write(f"{N};{alpha};{t_end};{swap_prob};TIMEOUT;300+\n")
            except Exception as e:
                print(f"CRITICAL ERROR: {e}")
                break

    print("\nExperiment finished.")

if __name__ == "__main__":
    main()