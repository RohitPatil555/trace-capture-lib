import bt2
import sys
from analysis_sink import ExampleSinkPlugin
import argparse

def execute_graph(input_file):
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

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Example CLI program")

    parser.add_argument("--input_file", required=True, help="Input file path")

    args = parser.parse_args()

    print(args.input_file)
    execute_graph(args.input_file)
