# Trace Capturing Library – A Practical Guide

In many projects it becomes essential to look back at what happened inside a running system, especially when bugs are hard to reproduce or performance problems hide behind layers of abstraction.

A trace is simply a record of events that have occurred during execution: function entries and exits, loop iterations, error codes, timestamps. By collecting this information you can replay the run later, examine the exact order in which things happened, and spot subtle timing issues or logic errors.

## Why use Babeltrace?

Linux already ships with an excellent tracing framework called Babeltrace.

It follows the Common Trace Format (CTF) specification – a compact binary format that is easy to write to from firmware and efficient to read on a PC.  Once your firmware emits CTF traces, you can plug them into Babeltrace’s powerful processing pipeline:

| Pipeline component	| What it does |
|:---------------------:|:-------------|
| Source	            |Reads trace data (from file or socket). In our case we use the built‑in CTF source.|
| Filter	| Lets you examine, transform or filter events. For example, we can watch a loop and capture every missing index. |
| Sink	 | Produces output – a console printer, a JSON writer, or any other custom format.|

With this model you write only one small plugin for Babeltrace (the sink), then everything else is handled by the library.

## The Trace Capturing Library

Our own library sits between firmware and Babeltrace. It has two key goals:

* **Easy integration** – Firmware developers can sprinkle trace calls into their code without worrying about CTF details.

* **Safety & ergonomics** – Written in modern C++20, it uses concepts to catch common mistakes at compile time.
Feeding data
The library accepts trace descriptions in a simple YAML file.

A developer writes something like:

```{.yaml}
traces:
  - name: loop_start
    fields:
      iteration: uint32
  - name: loop_end
    fields:
      iteration: uint32
```

The code generator inside the library produces C++ helper functions that emit these events as CTF packets.

Because the YAML is parsed at build time, any typo or type mismatch will surface early – you’ll never accidentally write an event with a missing field.

#### Post‑analysis

Once your firmware has been running for a while and has written trace files to disk (or streamed them over a socket), the only thing left is to feed those files into Babeltrace.

A single line of command, something like:

```{.bash}
babeltrace --output=stdout my_trace.ctf
```

will run the source → filter → sink pipeline and print human‑readable information on your console.


## Sample Example

Imagine we have two traces:
* One trace marks when a loop starts and stops.
* The other trace records every index of the loop.

We will use **babeltrace** to find out how long the loop ran and whether any indices were missing.

In this example we purposely skip recording index 5 so that we can see how the script reports it as missing. The code for this example lives in the _“example”_ folder.

### Trace yaml file

Below is a YAML file describing the two events:

* **loopTime** – has a `state` field, `1` means the loop started, `0` means it stopped.
* **loopCount** – records the loop index in a `count` parameter.

```yaml
- traces:
  - name: loopCount
    id: 1
    params:
      - name: count
        type: uint8_t
  - name: loopTime
    id: 2
    params:
      - name: state
        type: uint8_t
```

### Application Code

The following snippet shows how the two traces are pushed for later analysis.

```c
	timeParam->state = 1;
	inst->pushTrace( &loopTime ); // loopTime with state 1 means loop started

	for ( idx = 0; idx < maxLoopCount; idx++ ) {
		/* Update the counter and push the trace. */
		param->count = idx + 1;
		if ( param->count == 5 ) {
			continue;               // skip index 5
		}
		inst->pushTrace( &loopIdx ); // loopCount trace to collect count parameter.
	}

	timeParam->state = 0;
	inst->pushTrace( &loopTime ); // loopTime with state 0 means loop stopped
```

### Babeltrace Plugin

We write a custom plugin that finds missing indices and calculates the loop time.

```python
@bt2.plugin_component_class
class ExampleSinkPlugin(bt2._UserSinkComponent):
    def __init__(self, config, params, obj):
        self._port = self._add_input_port("in")
        self.start_time = 0
        self.diff_time = 0
        self.next_index = 0

    def _user_graph_is_configured(self):
        self._it = self._create_message_iterator(self._port)

    def _user_consume(self):
        # Consume one message and print it.
        msg = next(self._it)

        if type(msg) is bt2._EventMessageConst:
            ts = msg.default_clock_snapshot.value
            name = msg.event.name
            if name == "loopTime":
                payload = msg.event.payload_field
                if payload["state"] == 1:
                    self.start_time = ts
                    self.next_index = 1
                else:
                    self.diff_time = ts - self.start_time
                    self.next_index = 0
                    print(f"Time for loop : {self.diff_time}")
            elif name == "loopCount":
                payload = msg.event.payload_field
                count = int(payload["count"])
                if count == self.next_index:
                    self.next_index += 1
                else:
                    print(f"Missing count is {self.next_index}")
                    self.next_index = count + 1
```

### Final Babeltrace pipeline

Build the babeltrace pipeline to analyze the trace log as shown below.

```python
    # Build processing pipeline using graph project
    graph = bt2.Graph()

    # create source as CTF filesystem
    comp_src = bt2.find_plugin('ctf').source_component_classes['fs']
    src_comp = graph.add_component(comp_src, 'CTF source', params={
            'inputs': [input_file]
        })

    # create sink using custom plugin myanalysis
    sink_comp = graph.add_component(ExampleSinkPlugin, 'my sink')

    for port in src_comp.output_ports.values():
        graph.connect_ports(port, sink_comp.input_ports['in'])

    # execute graph
    graph.run()
```

### Execute and Output

* Execute below command

```{bash}
./test_ci.sh --example
```

* Output as
```text
Missing count is 5
Time for loop : 16274 nsec
```


## Conclusion

This shows how to design a custom sink and turn raw trace data into useful information.

You can also write a plain Python script that reads the trace files directly without using the graph or plugin system. For more details, visit the [babeltrace website](https://babeltrace.org/docs/v2.0/python/bt2/examples.html#iterate-trace-events).

## Next Action

In the next activity we will look at an STM32‑based embedded application and use this tracing technique to learn more about its behavior.

See you in the next chapter!
