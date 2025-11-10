# QTproject
## Hopfield Fractional Network Simulator (Qt GUI)

This project is a **Qt-based GUI tool** for building and simulating **Hopfield neural networks** using:

- Ordinary Differential Equation (ODE) solver  
- Fractional-order (Gamma-based) solver

Users can visually design a 5-node network, assign activation functions (sin, tanh, ReLU), set custom weights, and run the simulation with real-time output.

---

##  Features

-  **5-node network** (y₁ to y₅)
-  **Visual GUI node editor**
-  **Custom activation function per connection** (sin, tanh, relu)
-  **Choose solver:** ODE or Fractional (Gamma)
-  **Live output on right panel**
-  **Graph plotting** with Gnuplot
-  **Export equations** and **result table**

---

## ⚙️ How It Works

```txt
1. User creates nodes (1–5) on canvas
2. Connect nodes and assign:
     - weight (sᵢⱼ)
     - function: sin / tanh / relu
3. Select:
     - Solver: ODE or Gamma
     - Time steps (t_max)
     - Constants α₁, α₂, α₃
4. Qt internally simulates:
     - ODE: Euler method
     - Gamma: Caputo fractional approx.
5. Result shown on right (text table)
6. Optional: Graph plotted via Gnuplot
