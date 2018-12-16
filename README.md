## Testing Cache Coherence Performance with Different Interconnect Networks

### URL

https://abhinavgirish.github.io/cache-coherence-final-project.github.io/

### Summary

We plan on building a well documented directory based cache coherence simulator with multiple interconnect layouts. We will be documenting and extending Elliot's previous 15-618 project, which is a snoop-based MESI cache coherence simulator. The end goal is to compare the performance of directory based cache coherence on different interconnect networks.  

### Background

We are trying to study the effect of different interconnect networks on the number of hops required for cache coherence synchronization. In class, we studied the advantages and disadvantages of different interconnect networks. Thus, we like to build a directory based cache coherence simulator with the following interconnects: Mesh, Ring, Fat-Tree, Torus, Crossbar, and Multi-stage logarithm. We like to compare the result with the theoretical advantages studied in class.

### Challenge

We will build off of a previous 15-618 class project from Elliot Lockerman. One key challenge we have to overcome is understand his code and document his code. In addition, he suggested that there maybe minor bugs that needed to be fix. Thus, this will be the biggest challenge. 

### Resources

We plan on using our own computers, since we’re planning on running simulations with traces rather than on actual machines. We also plan to use starter code from Elliot Lockerman's 15-618 project 2 years ago (http://elliot.lockerman.info/projects/parallel_project/).

### Goals and Deliverables

Our goals stated in the proposal we submitted were scrapped since the project has changed. Our deliverables are now as follows:
- Implementation of directory-based coherence simulation using multiple (mesh, torus, and ring, fat tree)
- Graph comparing the performance (defined as number of hops) of simulation with the theoretical performance optimization 
- Well-written documentation of the simulator so that future 418/618 students can build off of it

Below are our nice-to-haves:
- Fix the migratory cache coherence implementation
- Create a simulation for other cache-coherence protocols like MOESI



### Platform Choice

We will be using Elliot Lockerman's project from 15-618 2 years ago. Since his starter code is coded in C++, we will be extending it with C++ as well.

### Schedule


- Week 3:  Study the start code and create detailed documentation for the starter code
- Week 4: Extend the starter code to different interconnect networks
- Week 5: Run tests on different traces to gather data
- Week 6: Extend to the framework with other cache-coherence protocols and fix migratory cache coherence.

### Checkpoint Report

[Link](https://docs.google.com/document/d/1ElAP2HD-56tSKEJwdNr6il35JUB-fCGlqjlQtVjIaS4/edit?usp=sharing)

### Final Project Report

For results and analysis of our experiment, please check out our report at the following link:

[Link](https://docs.google.com/document/d/1xTGnUswOhQo_9LrA9M8nr_O6-lnXVfZjN3fjbsmtBQI/edit?usp=sharing)

