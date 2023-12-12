"""
Compresses a DEFLATE compressed file with a fixed or dynamic Huffman tree.
"""

def compressed_file(
        name,
        src,
        strategy = "fixed"):
    """
    Compresses a file with the DEFLATE algorithm and a fixed or dynamic Huffman tree.

    Args:
      name: string
        Name for compressed_file rule.
      src: string_label
        Decompressed file.
      strategy: string
        one of [`fixed`, `dynamic`].

        Determines if compression should use a fixed Huffman tree or a dynamic
        Huffman tree.
    """
    if strategy not in ["fixed", "dynamic"]:
        fail()

    tools = ["//tools:deflate_compress"]

    native.genrule(
        name = "gen_" + name,
        srcs = [src],
        outs = [name],
        tools = tools,
        cmd = "$(execpath {tool}) --src $< {strat} > $@".format(
            tool = tools[0],
            strat = "--fixed" if strategy == "fixed" else "",
        ),
    )
