## Testing Cache Coherence Performance with the Spandex Protocol Enhancement

### URL

https://github.com/abhinavGirish/spandex-cache-coherence.github.io

### Summary

We plan on implementing different cache protocols and examining their performances across various cache protocol enhancements, including the Spandex protocol from ISCA. We will be utilizing/extending the ZSIM library to do a simulation of said protocols and seeing whether or not it meets expectations of its performance, based on findings from papers.

### Background

We are trying to compare the effectiveness of the cache coherence protocol improvements. The particular coherence protocol improvement that we are interested in is the Spandex protocol published in the proceedings of ISCA. The idea of the Spandex protocol, named after the flexible material, is to provide a flexible interface that extends existing coherence protocols without the need of any change to the memory structure. The paper claims that the Spandex protocol observed an 16% decrease in execution time on average and 27% decrease network traffic on average when compared to MESI.

### Challenge

The problem is challenging because creating an abstraction for a single shared address space for multiple caches, as we have learned in class. The most challenging part is to understanding the source code of zsim and expand upon its functionality. In addition, implementing the Spandex protocol could be challenging.

### Resources

We plan on using our own computers, since we’re planning on running traces on simulations rather than on actual machines. We also plan to use starter code from the Zsim library - tentatively, we plan on getting starter code from online tutorials and resources, which are available on the following site:  http://zsim.csail.mit.edu/tutorial/. We are using the following paper as a reference: http://rsim.cs.illinois.edu/Pubs/18-ISCA-Spandex.pdf. If necessary, we could try reaching out to Professor Beckmann for help with understanding Zsim.

### Goals and Deliverables

Poster Session (PLAN TO ACHIEVE):
- Graphs of execution time, network traffic visualization for all the trace file tests we do - We think this is feasible because running the tests and graphing the results should not take too long.
- Diagram of zsim implementation for both MESI and Spandex - Understanding how Zsim works is critical to our project and is one of the first things we are going to work on - this should be doable within the first couple of weeks of the project.
- Diagram of Spandex protocol enhancement (handling reads/writes/loads/stores of data etc.) - This seems feasible because the protocol is described in the handout.

HOPE TO ACHIEVE:
- Implementation of graph algorithms described in the paper that were used for testing, running said algorithms on both MESI and Spandex and seeing the analysis

### Platform Choice

We are using the Zsim framework, written in C++. It makes sense to use this system because building from scratch would be quite cumbersome, and the framework already has a few built-in functions and features to represent cache lines, invalidations, accesses, states etc. According to the website, it “can simulate multicore systems with detailed out-of-order cores and complex, heterogeneous memory hierarchies at speeds of 10s to 100s of MIPS”.

### Schedule

- Week 1: Explore the Zsim library and its capabilities; read the ISCA paper and jot down notes on implementation - see how zsim’s capabilities can be used
- Week 2: Implement the MESI protocol states, response types, and transitions
- Week 3:  Implement states (I, V, S, O), request types (Inv, RvkO), response types (RspRvkO, Ack) and transitions described in the paper
- Week 4: run trace file tests for network traffic, execution time and run them on both MESI and 
- Week 5: If time permits, implement graph algorithms in paper; otherwise, wrap up network traffic and execution time tests
- Week 6: Put together poster of results, analysis, and comparison with expectations of performance
