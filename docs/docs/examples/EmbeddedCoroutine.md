# Embedded Co-routine

In this post, we'll take a look at C++20 co-routine analysis using the trace-capture-lib.

## High Level View

• We have bare-metal co-routine code written in C++20.
• It emits an event every time a co-routine is switched.

![img](example/embd_example.png)

## Script

Since the events are now captured in Babeltrace, we can write a script that generates a Gantt chart showing how the co-routines are scheduled.

__Note: The timing appears constant because we use simulated timestamps rather than an actual hardware timer__

## Conclusion

This demonstrates how the library can be used in embedded or firmware systems to analyze behavior offline.

## Next Topic

See the Release page for upcoming updates.
Bug reports and feedback are welcome — thank you for your support in helping improve the project.
