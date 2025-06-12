# COMP410 Assignments Overview

This repository contains three OpenGL-based assignments for COMP410, each focusing on different aspects of computer graphics and interactive programming. Below is a summary of each assignment, its main features, and user controls.

---

## Assignment 1: Bouncing Shape Simulation

**Description:**
- Simulates a bouncing circle (or square) under gravity, with realistic physics and collision handling.
- The shape can be toggled between a filled/unfilled circle or square, and its color can be switched between red and blue.
- The trajectory of the bouncing shape can be displayed as a series of small shapes.

**Main Features:**
- Realistic 2D physics: gravity, restitution (bouncing), and friction.
- Toggle between circle and square shapes.
- Toggle between filled and outline rendering.
- Color switching (red/blue).
- Trajectory visualization.

**Controls:**
- `Q`: Quit the application
- `I`: Reset position to top left
- `C`: Toggle color (red/blue)
- `T`: Toggle trajectory display on/off
- **Left Mouse Button**: Toggle filled/outline shape
- **Right Mouse Button**: Toggle between circle/square
- `H`: Display help message

---

## Assignment 2: Interactive Rubik's Cube (3x3x3)

**Description:**
- Renders and allows interaction with a 3x3x3 Rubik's Cube.
- Users can rotate the entire cube, rotate individual slices, and scramble the cube with random moves.
- Each subcube is rendered with appropriate face colors and black for internal faces.

**Main Features:**
- Full 3D Rubik's Cube rendering with colored faces.
- Mouse-based cube rotation.
- Keyboard controls for rotating individual slices (clockwise/counter-clockwise) along X, Y, and Z axes.
- Scramble function for randomizing the cube.

**Controls:**
- **Mouse:**
  - Left-click and drag: Rotate the entire cube
- **Keyboard:**
  - `h`: Display help message
  - `q`/`ESC`: Quit the application
  - `s`: Scramble the cube (20 random moves)
- **Slice Rotation Controls:**
  - **X-axis:**
    - `f`/`c`: Front slice clockwise/counter-clockwise
    - `m`/`n`: Middle X slice clockwise/counter-clockwise
    - `b`/`v`: Back slice clockwise/counter-clockwise
  - **Y-axis:**
    - `t`/`y`: Top slice clockwise/counter-clockwise
    - `g`/`j`: Middle Y slice clockwise/counter-clockwise
    - `u`/`i`: Bottom slice clockwise/counter-clockwise
  - **Z-axis:**
    - `l`/`k`: Left slice clockwise/counter-clockwise
    - `o`/`p`: Middle Z slice clockwise/counter-clockwise
    - `r`/`e`: Right slice clockwise/counter-clockwise

---

## Assignment 3: Shading and Texturing a Bouncing Sphere

**Description:**
- Demonstrates the difference between Gouraud (per-vertex) and Phong (per-fragment) shading on a 3D sphere.
- Supports texturing (basketball and earth textures), material switching (plastic/metallic), and dynamic lighting.
- The sphere bounces with realistic physics and can be rotated, zoomed, and paused.

**Main Features:**
- Toggle between Gouraud and Phong shading (with separate shaders).
- Switch between plastic and metallic materials.
- Apply different textures (basketball, earth) or show wireframe.
- Fixed or moving light source.
- Realistic bouncing animation with pause and reset.
- Zoom and rotation controls.

**Controls:**
- `ESC`/`Q`: Exit program
- `R`: Reset sphere position
- `S`: Toggle between Gouraud and Phong shading
- `O`: Change shading mode (ambient/diffuse/specular)
- `M`: Toggle between plastic and metallic materials
- `Z`: Zoom in
- `W`: Zoom out
- `L`: Toggle between fixed and moving light
- **Arrow Keys**: Rotate sphere
- `I`: Toggle between basketball and earth textures
- `T`: Toggle texture display mode (no texture/texture/wireframe)
- `SPACE`: Pause/resume animation
- `K`: Toggle self-rotation
- `H`: Show help message

---

For more details, see the source code and comments in each assignment's directory. 