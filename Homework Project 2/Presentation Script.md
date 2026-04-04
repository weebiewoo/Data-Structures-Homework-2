# CSD2183 Data Structures — Project 2
## Area- and Topology-Preserving Polygon Simplification
### 10-Minute Video Presentation Script

---

> **Speaker key:** [S1] = Speaker 1 | [S2] = Speaker 2 | [S3] = Speaker 3
> **Timing:** Each section includes an approximate target time.
> **Note:** Replace [YOUR GROUP NAME/NUMBER] with your actual group identifier.

---

## SLIDE 1 — Title Slide (~30 sec)

**[S1]**
Hi everyone. We're [YOUR GROUP NAME], and today we're presenting Project 2 for CSD2183 Data Structures — Area- and Topology-Preserving Polygon Simplification. In this video we'll walk you through the problem, our solution and the data structures behind it, and a performance evaluation of our implementation.

---

## SLIDE 2 — The Problem (~90 sec)

**[S1]**
So what's the problem we're solving?

Imagine you have a geographic polygon — say, the boundary of a country on a map — defined by thousands of vertices. Rendering or querying that shape in real time is expensive. You want to use fewer vertices to make it cheaper, but you can't just throw vertices away. You have two strict constraints.

First: **area preservation**. The total area enclosed by the exterior ring, and the area of each individual hole, must be kept exactly the same. This matters in applications like the Geographic Information System (GIS), where a polygon might represent a land parcel — you can't just shrink it.

Second: **topology preservation**. The number of rings must stay the same. No ring can cross itself. No ring can cross another ring. The shape must stay geometrically valid. It must have clean boundaries, no weird self-intersections, holes properly nested inside the outer shape.

And on top of those two hard constraints, you want to **minimize areal displacement** — that is, minimize how much the shape visually shifts from the original, measured as the area of the symmetric difference between the old and new polygon.

So the challenge is: simplify aggressively while breaking nothing.

---

## SLIDE 3 — What is APSC? (~90 sec)

**[S2]**
To solve this, we implemented the **Area-Preserving Segment Collapse**, or APSC, algorithm, described by Kronenfeld et al. (2020).

Here's the key idea. Take any four consecutive vertices in a ring — call them A, B, C, D. In the current polygon, the path goes A → B → C → D. APSC asks: what if we collapsed B and C into a single new vertex E, replacing the path with A → E → D?

If E is chosen randomly, the area changes. But APSC computes E analytically — using the intersection of two lines — such that the area added on one side of the new segment exactly cancels the area removed on the other side. The result is a collapse that is, by construction, **area-neutral**.

The cost of a particular collapse is its **local areal displacement** — how much the shape visually deforms from that one operation. APSC always picks the cheapest valid collapse available across all rings, applies it, and repeats until the target vertex count is reached. This greedy strategy is the core loop.

[SHOW DIAGRAM of A→B→C→D collapsing to A→E→D]

The critical constraint is that a collapse is only valid if: the area is preserved within floating-point tolerance, the new edges A→E and E→D don't intersect any existing edges, and the full topology of the polygon — ring simplicity and ring-ring separation — still holds after the change.

---
## advantage disadvantage
"The advantages of APSC are: it explicitly preserves the enclosed area of the polygon, which makes it great for geographic applications like land parcels or urban boundaries where area accuracy really matters. It also produces smoother, more natural-looking results compared to algorithms that just drop points, which can leave the shape looking jagged or sunken.
The disadvantages of APSC are: first, it generates new synthetic points that didn't exist in the original polygon, which can cause compatibility issues in pipelines that expect the simplified polygon to only use original vertices. Second, it is harder to implement than alternatives like RDP or Visvalingam, since you need extra geometric reasoning to compute the area-preserving point. And third, it is somewhat more computationally expensive per operation, which becomes a real concern at massive scale, like simplifying millions of polygons in a GIS dataset — though for small to medium polygons you'd barely notice the difference."

## SLIDE 5 — Algorithm Walkthrough (~60 sec)

**[S2]**
Let's trace through the main loop quickly.

`simplify_polygon` runs while the total vertex count exceeds the target. Each iteration scans every ring and, for each ring, evaluates every window of four consecutive vertices. For each window, it:
1. Computes the area-preserving point E via line intersection,
2. Checks that the two new edges don't intersect any existing edge,
3. Applies the collapse tentatively and verifies the full topology,
4. Records the candidate if it's valid and tracks the one with the lowest cost.

After scanning all rings, the globally cheapest valid candidate is applied, the ring's vertex array is updated, and the cost is added to the running total. This repeats until the target is met or no valid collapse exists.

---

## SLIDE 6 — Design Decisions & Trade-offs (~60 sec)

**[S3]**
We made a deliberate choice to keep the implementation **correct and readable first**. Rather than a priority queue or spatial index, we do a full re-scan every iteration. This means the time complexity is **O(N³) per collapse step per collapse step** — O(N) windows times an O(N) topology check — and **O(N³) overall** in the worst case.

For the polygon sizes in the provided test cases, this is perfectly fast. The benefit is that the code is easy to verify for correctness, and our topology checks are thorough: we validate ring simplicity, ring-ring intersections, and hole containment after every single collapse.

The honest trade-off is that for inputs with 100,000 or more vertices, a priority queue combined with incremental updates — rather than re-scanning from scratch each iteration — would be significantly faster. That would bring the complexity down toward O(N log N). That's a natural next step for scaling this solution.

---

## SLIDE 7 — Experimental Evaluation (~90 sec)

**[S1]**
We tested our implementation on [X] test cases, including the reference cases provided and our own custom inputs.

[SHOW RESULTS TABLE or PLOT]

For **correctness**: every test case produced output where the total signed area matched the input to within 1e-8, and all topology constraints — no self-intersections, no ring crossings — were satisfied.

For **running time**: [INSERT YOUR ACTUAL MEASUREMENTS HERE — e.g., "On an input with 84 vertices, the algorithm completed in under 1 millisecond. On our stress-test input with [N] vertices, it completed in [T] seconds."]

For **areal displacement quality**: [INSERT YOUR MEASUREMENTS — e.g., "Reducing from 84 to 20 vertices produced a total areal displacement of approximately 0.016 square units, matching the expected output."]

[SHOW PLOT: running time vs. input size]

The running time grows roughly as **O(N²)** with input size, consistent with our analysis. Memory usage scales linearly with vertex count, as expected from our vector-based representation.

---

## SLIDE 8 — Conclusion (~30 sec)

**[S1], [S2], [S3]** *(hand off naturally)*

**[S3]**
To summarize: we implemented the APSC algorithm using a clean, vector-based polygon representation with a `CollapseCandidate` struct driving a greedy simplification loop. Our implementation correctly preserves area and topology across all test cases.

**[S2]**
The core strength is correctness and readability. The natural extension for very large inputs would be to add a priority queue for O(log N) candidate selection and incremental updates to avoid rescanning the whole polygon each step.

**[S1]**
Thanks for watching. The full source code, test cases, and documentation are available in our GitHub repository.

---

## SLIDE GUIDE (for reference when building slides)

| Slide | Title | Speaker | Key visual |
|-------|-------|---------|------------|
| 1 | Title / Group Name | S1 | Group name, course code |
| 2 | The Problem | S1 | Polygon with holes diagram; before/after simplification |
| 3 | What is APSC? | S2 | A→B→C→D → A→E→D diagram |
| 4 | Our Data Structures | S3 | Code snippets: Point, Ring, Polygon, CollapseCandidate structs |
| 5 | Algorithm Walkthrough | S2 | Pseudocode or flowchart of the main loop |
| 6 | Design Decisions | S3 | Complexity table: our approach vs. priority queue approach |
| 7 | Experimental Results | S1 | Plots / tables of running time and areal displacement |
| 8 | Conclusion | All | Summary bullet points |

---

*Total estimated speaking time: ~9 min 30 sec — leaves ~30 sec buffer.*
*Word count: ~1,050 spoken words at ~110 words/min = ~9.5 minutes.*
