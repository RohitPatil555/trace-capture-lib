# SPDX-License-Identifier: MIT | Author: Rohit Patil
#!/usr/bin/env python3
import sys
import os
import yaml
import textwrap
from jinja2 import Template

# Supported C/C++ integer types that can appear in the trace definitions.
_supported_type_list = ["uint8_t", "uint16_t", "uint32_t", "int8_t", "int16_t", "int32_t"]

# --------------------------------------------------------------------------- #
# Generic file generator ---------------------------------------------------- #
# --------------------------------------------------------------------------- #
class GenerateFile:
    """
    Base class that handles writing a header file and appending traces to it.
    The file path is stored in ``outfile`` and the stream ID (used in templates)
    is stored in ``stream_id``.
    """
    def __init__(self, file_path, streamId):
        self.outfile = file_path
        self.bHeaderAdded = False
        self.stream_id = streamId

    # --------------------------------------------------------------------- #
    # Header generation --------------------------------------------------- #
    # --------------------------------------------------------------------- #
    def add_header(self, tmpl_str):
        inputs = {}
        inputs["stream_id"] = self.stream_id
        template = Template(tmpl_str)
        with open(self.outfile, 'w+') as f:
            out_str = template.render(**inputs)
            f.write(out_str)

    # --------------------------------------------------------------------- #
    # Trace generation ---------------------------------------------------- #
    # --------------------------------------------------------------------- #
    def add_trace(self, tmpl_str, trace):
        inputs = {}
        inputs["stream_id"] = self.stream_id
        inputs["trace"] = trace
        template = Template(tmpl_str)
        with open(self.outfile, 'a') as f:
            out_str = template.render(**inputs)
            f.write(out_str)

# --------------------------------------------------------------------------- #
# C++ header file generator ------------------------------------------------- #
# --------------------------------------------------------------------------- #
class CppHeaderFile(GenerateFile):
    def __init__(self, dpath, streamId):
        super().__init__(f"{dpath}/trace_types.hpp", streamId)
        self._create()

    # --------------------------------------------------------------------- #
    # Header skeleton ----------------------------------------------------- #
    # --------------------------------------------------------------------- #
    def _create(self):
        """
        Write a minimal header that includes the common trace definition,
        guards, and defines ``TRACE_STREAM_ID``.
        """
        c_code_tmpl = """

        #include <trace.hpp>

        #pragma once

        #define TRACE_STREAM_ID  {{ stream_id }}

        """
        clean_template = textwrap.dedent(c_code_tmpl)
        super().add_header(clean_template)

    # --------------------------------------------------------------------- #
    # Trace type definition ----------------------------------------------- #
    # --------------------------------------------------------------------- #
    def addTrace(self, trace):
        """
        Append a struct and ``TraceId`` specialization for the given trace.
        The struct is marked with ``__attribute__((packed))`` to avoid
        padding between fields.
        """
        c_code_tmpl = """
        typedef struct {
            {%- for f in trace.params %}
            {%- if f.count is defined %}
            {{ f.type }} {{ f.name }}[{{ f.count }}];
            {%- else %}
            {{ f.type }} {{ f.name }};
            {%- endif %}
            {%- endfor %}
        } __attribute__((packed)) {{ trace.name }}_t;

        template <>
        struct TraceId<{{ trace.name }}_t> {
            static constexpr uint32_t value = {{ trace.id }};
        };
        """
        clean_template = textwrap.dedent(c_code_tmpl)
        super().add_trace(clean_template, trace)

# --------------------------------------------------------------------------- #
# Babeltrace metadata generator --------------------------------------------- #
# --------------------------------------------------------------------------- #
class BabeltraceMetadata(GenerateFile):
    """
    Generates a Babeltrace (CTF) configuration file that describes the
    trace format and all traces.  The main file is named simply ``metadata``.
    """
    def __init__(self, dpath, streamId):
        super().__init__(f"{dpath}/metadata", streamId)
        self._create()

    def _create(self):
        """
        Write the core trace definition (types, trace properties,
        clock, packet header, etc.).  The only dynamic part is
        ``{{ stream_id }}``.
        """
        bb_config_hdr = """\
        /* CTF 1.8 */

        typedef integer { size = 64; align = 8; signed = false; } uint64_t;
        typedef integer { size = 32; align = 8; signed = false; } uint32_t;
        typedef integer { size = 16; align = 8; signed = false; } uint16_t;
        typedef integer { size = 8; align = 8; signed = false; }  uint8_t;
        typedef integer { size = 32; align = 8; signed = true; }  int32_t;
        typedef integer { size = 16; align = 8; signed = true; }  int16_t;
        typedef integer { size = 8; align = 8; signed = true; }   int8_t;

        trace {
            major = 1;
            minor = 8;
            byte_order = le;

             /* Packet header: must contain stream_id */
             packet.header := struct {
                 uint32_t stream_id;
             };

        };

        clock {
             name = myclock;
             freq = 1000000000; /* 1 GHz = ns */
        };

        stream {
             id = {{ stream_id }};

             packet.context := struct {
                 uint64_t timestamp_begin;
                 uint64_t timestamp_end;
                 uint32_t traces_discarded;
                 uint32_t packet_size;
                 uint32_t content_size;
                 uint32_t packet_seq_count;
             };

             event.header := struct {
                 uint32_t id;
                 uint64_t timestamp;
             };
        };

        """
        clean_template = textwrap.dedent(bb_config_hdr)
        super().add_header(clean_template)

    # --------------------------------------------------------------------- #
    # Individual trace definition ----------------------------------------- #
    # --------------------------------------------------------------------- #
    def addTrace(self, trace):
        """
        Append an ``trace`` block to the CTF configuration.  Each trace
        contains its name, id, stream ID and a struct of fields.
        """
        bb_config_trace = """
        event {
            name = {{ trace.name }};
            id   = {{ trace.id }};
            stream_id = {{ stream_id }};

            fields := struct {
                {%- for f in trace.params %}
                {%- if f.count is defined %}
                {{ f.type }} {{ f.name }}[{{ f.count }}];
                {%- else %}
                {{ f.type }} {{ f.name }};
                {%- endif %}
                {%- endfor %}
            };
        };

        """
        clean_template = textwrap.dedent(bb_config_trace)
        print(trace)
        super().add_trace(clean_template, trace)

# --------------------------------------------------------------------------- #
# YAML parsing utilities ---------------------------------------------------- #
# --------------------------------------------------------------------------- #
def parse_yaml_file(file_path):
    """
    Generator that yields one trace at a time from a potentially large
    YAML file.  The YAML is expected to be a list of dictionaries,
    each containing an ``traces`` key whose value is a list of traces.
    """
    with open(file_path, 'r') as f:
        data = yaml.safe_load(f)

    for trace_entry in data:
        traces = trace_entry.get('traces', [])
        for ev in traces:
            yield (ev)

# --------------------------------------------------------------------------- #
# Validation utilities ------------------------------------------------------ #
# --------------------------------------------------------------------------- #
def check_argument(trace):
    """
    Validate that the trace dictionary contains a string name, a non‑negative
    integer ID and parameters that use only supported types.
    If validation fails, print an error message and exit.
    """
    trace_name = trace['name']
    trace_id = int(trace['id'])
    if not isinstance(trace_name, str):
        print(f"group:{gName} trace name not a string")
        sys.exit(-1)
    if trace_id < 0:
        print(f"{trace_name} trace Id negative not supported")
        sys.exit(-1)

    params = trace.get('params', [])
    if params is not None and isinstance(params, list) and len(params) != 0:
        for p in params:
            t = p['type']
            n = p['name']
            if not isinstance(n, str):
                print(f"group:{gName} trace:{trace_name} parameter name is not as type string {n}")
                sys.exit(-1)
            if not isinstance(t, str):
                print(f"group:{gName} trace:{trace_name} parameter {n}: type is not in string {t}")
                sys.exit(-1)
            if 'count' in p:
                c = p['count']
                if not isinstance(c, int):
                    print(f"group:{gName} trace:{trace_name} parameter {n}: count don't have type integer {c}")
                    sys.exit(-1)
                if c <= 0:
                    print(f"group:{gName} trace:{trace_name} parameter {n} count not allow as negative or zero {c}")
                    sys.exit(-1)
            if t not in _supported_type_list:
                print(f"group:{gName} trace:{trace_name} have unsupported type {t}")
                sys.exit(-1)

# --------------------------------------------------------------------------- #
# Main entry point ---------------------------------------------------------- #
# --------------------------------------------------------------------------- #
def main(yaml_file, out_path):
    """
    High‑level driver that creates the C++ header and Babeltrace
    metadata files, then iterates over all traces in the YAML file,
    validates them, and writes their definitions to both outputs.
    """
    c_file = CppHeaderFile(out_path, 0)
    bb_file = BabeltraceMetadata(out_path, 0)

    for trace in parse_yaml_file(yaml_file):
        check_argument(trace)
        c_file.addTrace(trace)
        bb_file.addTrace(trace)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <yaml_file>")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])
