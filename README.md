# example-vulkan

Vulkan's sample programs are not so many and most of them make it difficult to understand what dependencies are between handles, so we try to make the dependencies as easy as possible.

The program does not cache, if possible, creates a handle when necessary, and releases it after use.