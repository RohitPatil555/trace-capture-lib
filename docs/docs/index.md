# Introduction

Trace logging in embedded systems is essential for debugging issues such as performance bottlenecks or race conditions. It also provides a high-level view of the system’s internal behavior.

This library provides a framework for collecting and sending traces over interfaces like UART or USB to a host system. It is implemented following the [Common Trace Format (CTF) specification](https://diamon.org/ctf/).

For analysis, you can use existing tools like [Babeltrace](https://babeltrace.org/), which supports Python, making it easy to automate data collection and visualize system behavior.

The library is written in C++20 and requires some integration effort into your application, as shown in the provided example.
