# Artifact for Type Error Localization

This is the artifact for "Total Type Error Localization and Recovery with Holes", submitted to the
POPL 2024 Artifact Evaluation process.

There are three separate parts to this artifact:

1.  [formalism.pdf](./formalism.pdf) contains the complete formalism for the marked lambda calculus
    and its extensions, the augmented Hazelnut action semantics, and appendices for the type hole
    inference work. See the preface for more detail.

2.  [agda/](./agda/) contains the Agda mechanization of the marked lambda calculus. See [its
    README](./agda/README.md) for more detail. The mechanization is also discussed briefly in
    Section 2.2 of the paper.

    -   The current repository may be found at
        <https://github.com/hazelgrove/error-localization-agda>

3.  [hazel/](./hazel/) contains a snapshot build of the Hazel implementation with type hole
    inference support. For more detail, see [its README](./hazel/README.md).
