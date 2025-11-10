# QTproject
# Hopfield Fractional Network Simulator (Qt GUI)

This project is a **Qt-based GUI tool** for building and simulating **Hopfield neural networks** using both:
- Ordinary Differential Equation (ODE) solver
- Fractional-order (Gamma-based) solver

Users can visually design a 5-node network, assign custom activation functions (sin, tanh, ReLU), set weights, and run the simulation with real-time visualization.

---

##  Features

-  **Drag-to-Create Nodes** (max 5, representing y₁ to y₅)
-  **Draw connections** between nodes with configurable weights and functions
-  **Select solver type**: ODE or Fractional (Gamma)
-  **View real-time simulation output** in a side panel
-  **Graph output** with Gnuplot
-  **Export equations and result tables**

---

##  How It Works

```txt
1. User creates nodes (1–5) on canvas
2. User connects them and selects function: sin / tanh / relu
3. Each connection stores:
   - start & end node
   - weight value (user input)
   - activation function
4. User selects:
   - Solver type: "ODE" or "Gamma"
   - Time steps (t_max)
   - Alpha constants (α₁, α₂, α₃)
5. Qt internally runs simulation:
   - If ODE: uses Euler method
   - If Gamma: uses Caputo approximation
6. Result shown on right in readable text
7. Graphs plotted using Gnuplot

## Image
1. User creates nodes (1–5) on canvas
2. User connects them and selects function: sin / tanh / relu
3. Each connection stores:
   - start & end node
   - weight value (user input)
   - activation function
4. User selects:
   - Solver type: "ODE" or "Gamma"
   - Time steps (t_max)
   - Alpha constants (α₁, α₂, α₃)
5. Qt internally runs simulation:
   - If ODE: uses Euler method
   - If Gamma: uses Caputo approximation
6. Result shown on right in readable text
7. Graphs plotted using Gnuplot

##  QT Network Visualization

![QT Network](https://raw.githubusercontent.com/minoverse/QTproject/main/QT_network.jpg)




