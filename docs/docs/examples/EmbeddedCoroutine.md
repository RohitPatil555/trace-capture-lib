# Embedded Co-routine Analysis

In this post, we'll take a look at C++20 co-routine analysis using the `trace-capture-lib`.

## High-Level Overview

* We have bare-metal co-routine code written in C++20.
* It emits an event every time a co-routine is switched.
* All events are captured and saved to a UART file, as shown in the diagram below.
* The captured file is then fed into a Python script that analyzes the Babeltrace data and dumps the information in a timing graph format.
```python
# Refer to code here: example/stm32/src/task_timing_graph.py
```


![Trace capture sequence diagram](embd_example.png)

## Script Output

__Note: The timing appears constant because we use simulated timestamps rather than an actual hardware timer.__

![img2](timegraph.png)

## Conclusion

This demonstrates how the library can be used in embedded or firmware systems to analyze behavior offline.
Additionally, instead of relying solely on local analysis, the data can be fed to an AI via the Model Context Protocol (MCP) tool to accelerate the analysis process.

## Next Topic

See the Releases page for upcoming updates.
Bug reports and feedback are always welcome — thank you for your support in helping improve the project!
