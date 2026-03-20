# Host to Device Interface

This section explains how the host and device communicate to collect trace packets for offline analysis. It also describes the commands supported by the device to retrieve the required output from the trace logging module.

## Command / Response

The host is the main controller of the device. It sends commands, and the device always replies with a response.
Since responses can be larger than a single transfer, we assume that the underlying communication layer will take care of splitting the response into smaller parts and reassembling them when needed.

![Message Flow](../img/host_protocol_seq_flow.png)

Here we define command and response payload as shown below

![message format](../img/host_protocol_format.png)
