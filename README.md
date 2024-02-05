# Langton's Ant (RLR)

This is a C++ implementation of Langton's ant with RLR rules. It runs a given number of steps as quickly as possible and then saves a PNG
(and optionally a checkpoint) of the result. On a Ryzen 9 7950X with Arch Linux, it runs at about 250 million steps per second and takes about 70
minutes to do one trillion steps. I've computed 5 trillion steps so far (with more on the way) and there's still no highway.

This uses an expandable 2D grid. When the ant reaches an edge of the grid, a new grid is created with 4x the area of the old grid and the old grid is
moved to the center of the new grid.

## Usage

- `./langton`: 1000 steps, no checkpoint
- `./langton 1000000000`: one billion steps, no checkpoint
- `./langton 1000000000000 langton.zst`: one trillion steps, checkpoint stored in `langton.zst`
