import sys
from pathlib import Path
import bt2
import matplotlib.pyplot as plt
import pandas as pd

def print_event(event, timestamp):
    print(f"\n========================================")
    print(f"EVENT: {event.name} {timestamp}")
    print(f"========================================")

    # 1. Main Parameter Payload
    if event.payload_field:
        print("  [Payload Parameters]")
        for key, val in event.payload_field.items():
            print(f"    {key} = {val}")

    # 2. Specific Event Context (Parameters linked to this exact event variant)
    if event.specific_context_field:
        print("  [Specific Context]")
        for key, val in event.specific_context_field.items():
            print(f"    {key} = {val}")

    # 3. Common Context Parameters (e.g., CPU ID, Thread ID, Process Name)
    if event.common_context_field:
        print("  [Common Context]")
        for key, val in event.common_context_field.items():
            print(f"    {key} = {val}")

def ctf_to_dataframe(trace_path):
    events_data = { "task": [], "start": [], "end": [] }
    last_event_time = None

    # Create the trace iterator
    msg_it = bt2.TraceCollectionMessageIterator(trace_path)

    for msg in msg_it:

        # Check if the message is an event message
        if type(msg) is bt2._EventMessageConst:
            event = msg.event
            _clock_snapshot = msg.default_clock_snapshot
            _timestamp = _clock_snapshot.value

            taks_name = "task_%d"%(event.payload_field["task_id"])
            events_data["task"].append(taks_name)
            events_data["start"].append(_timestamp)

            if last_event_time == None:
                last_event_time = _timestamp
            else:
                events_data["end"].append(_timestamp)
                last_event_time = _timestamp

    events_data["end"].append(last_event_time + 10)

    return events_data

# Get the CTF trace directory path from command-line argument
def generate_plot(trace_path, output_path):
    data = ctf_to_dataframe(trace_path)

    print(data)

    df = pd.DataFrame(data)
    df['duration'] = df['end'] - df['start']

    fig, ax = plt.subplots(figsize=(10, 4))

    ax.barh(
        y=df['task'],
        width=df['duration'],
        left=df['start'],
        height=0.4,
        color='skyblue',
        edgecolor='royalblue'
    )

    ax.invert_yaxis()  # Puts task_0 at the top row instead of the bottom
    ax.set_xlabel("Time (Nanoseconds)")
    ax.set_ylabel("Tasks")
    ax.set_title("Task Timeline Execution Graph", fontsize=12, pad=15)

    ax.grid(axis='x', linestyle='--', alpha=0.7)

    plt.tight_layout()
    plt.savefig(output_path)

if __name__ == "__main__":
    # 1. Check if the user forgot to provide both arguments
    if len(sys.argv) < 3:
        print("Error: Missing required arguments.", file=sys.stderr)
        print("Usage: python script.py <trace_path> <output_image_path>", file=sys.stderr)
        print("Example: python script.py ./my_trace ./output_gantt.png", file=sys.stderr)
        sys.exit(1)

    # 2. Extract arguments
    raw_trace_path = sys.argv[1]
    raw_output_path = sys.argv[2]

    trace_path = Path(raw_trace_path)
    output_path = Path(raw_output_path)

    # 3. Validate input trace path
    if not trace_path.exists():
        print(f"Error: The input trace path does not exist -> '{raw_trace_path}'", file=sys.stderr)
        sys.exit(1)

    # 4. Validate output destination folder
    # output_path.parent extracts the directory block where the file will be saved
    output_dir = output_path.parent

    if not output_dir.exists():
        try:
            # Create the folder path automatically if it doesn't exist yet
            output_dir.mkdir(parents=True, exist_ok=True)
            print(f"Created missing output directory: {output_dir}")
        except Exception as e:
            print(f"Error: Cannot create output directory '{output_dir}'. Reason: {e}", file=sys.stderr)
            sys.exit(1)

    # 5. Core execution logic safely executes here
    print(f"Input Trace:  {trace_path.resolve()}")
    print(f"Output Chart: {output_path.resolve()}")

    generate_plot(raw_trace_path, raw_output_path)
