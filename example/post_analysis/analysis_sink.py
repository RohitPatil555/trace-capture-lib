import bt2

bt2.register_plugin(__name__, "myanalysis")

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
