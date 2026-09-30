# QTproject
## Hopfield Fractional Network Simulator (Qt GUI)

This project is a **Qt-based GUI tool** for building and simulating **Hopfield neural networks** using:

- Ordinary Differential Equation (ODE) solver  
- Fractional-order (Gamma-based) solver

Users can visually design a 5-node network, assign activation functions (sin, tanh, ReLU), set custom weights, and run the simulation with real-time output.


## Architecture

The project is separated into independent components for GUI handling,
five-node model definition, numerical solvers, validation, result output,
and plotting.

This separation makes the numerical implementation easier to inspect and
debug independently from the GUI.

### Refactoring Note

The refactoring preserves the existing numerical implementation, including
the equations, parameters, Gamma/ODE calculations, and calculation order.
Numerical behavior is validated against the original implementation after
each major separation step.

---

# Architecture

```text
                         ┌─────────────────────┐
                         │        GUI          │
                         │    ButtonNetwork    │
                         │                     │
                         │ buttons / dialogs   │
                         │ network drawing     │
                         └──────────┬──────────┘
                                    │
                     ┌──────────────┴──────────────┐
                     │                             │
              normal simulation             validation request
                     │                             │
                     ▼                             ▼
          ┌────────────────────┐       ┌─────────────────────┐
          │ FiveNodeParameters │◄──────│  ValidationRunner   │
          │                    │       │                     │
          │ alpha1/alpha2/     │       │ reference setup     │
          │ alpha3/nu          │       │ reference weights   │
          │ coefficients       │       │ validation workflow │
          │ initial values     │       └──────────┬──────────┘
          └─────────┬──────────┘                  │
                    │                             │
                    ▼                             │
          ┌────────────────────┐                  │
          │   FiveNodeModel    │◄─────────────────┘
          │                    │
          │ 5-node equations   │
          │ activation/gates   │
          └─────────┬──────────┘
                    │
                    ▼
          ┌────────────────────┐
          │       Solver       │
          │                    │
          │ Gamma calculation  │
          │ ODE calculation    │
          │ BG/Gamma weights   │
          └─────────┬──────────┘
                    │
              SimulationResult
                    │
          ┌─────────┴─────────────┐
          │                       │
          ▼                       ▼
 ┌──────────────────┐    ┌────────────────────┐
 │   ResultWriter   │    │    PlotManager     │
 │                  │    │                    │
 │ .dat             │    │ individual plots   │
 │ .csv             │    │ five-node plots    │
 │ tables           │    │ reference-style    │
 │ run information  │    │ plots / scans      │
 └──────────────────┘    └────────────────────┘
```

---

# Directory Structure

```text
ButtonNetwork/
│
├── main.cpp
├── ButtonNetwork.pro
├── README.md
│
├── gui/
│   ├── buttonnetwork.h
│   └── buttonnetwork.cpp
│
├── model/
│   ├── fivenodeparameters.h
│   ├── fivenodeparameters.cpp
│   ├── fivenodemodel.h
│   └── fivenodemodel.cpp
│
├── solver/
│   ├── solver.h
│   └── solver.cpp
│
├── validation/
│   ├── validationrunner.h
│   └── validationrunner.cpp
│
├── output/
│   ├── resultwriter.h
│   └── resultwriter.cpp
│
└── plot/
    ├── plotmanager.h
    └── plotmanager.cpp
```

---

# Component Responsibilities

## 1. GUI — `gui/ButtonNetwork`

`ButtonNetwork` is responsible only for user interaction and network visualization.

Responsibilities:

- create/edit network nodes
- create/edit connections
- mouse handling
- network drawing
- dialogs
- GUI controls
- collect user input
- request simulations
- display simulation status/results

The GUI should not contain the numerical Gamma or ODE implementation.

```text
User
  ↓
ButtonNetwork
  ↓
parameters / simulation request
  ↓
model + solver
```

---

## 2. Parameters — `model/FiveNodeParameters`

Contains the numerical parameters required by the five-node model.

Examples:

```text
alpha1
alpha2
alpha3
nu

initial conditions

y1(0)
y2(0)
y3(0)
y4(0)
y5(0)

coefficient/weight table

s12
s13
...
s52
```

This separation makes it possible to check model parameters independently from the equations and solver.

When debugging:

```text
Are the coefficients correct?
        ↓
FiveNodeParameters
```

---

## 3. Mathematical Model — `model/FiveNodeModel`

Contains the mathematical definition of the five-node system.

Responsibilities:

- five-node equations
- activation functions required by the model
- node-specific gate functions
- evaluation of the mathematical model

It should describe **what is being calculated**, not how the numerical solver iterates over time.

When debugging:

```text
Are the equations correct?
        ↓
FiveNodeModel
```

The model should not:

- draw GUI elements
- open dialogs
- generate plots
- write result files

---

## 4. Numerical Solver — `solver/Solver`

Contains the numerical algorithms.

Responsibilities:

### Gamma solver

- Gamma/Beta calculation
- BG weight calculation
- `om` loop
- `r` loop
- fractional accumulation
- initial-value addition

### ODE solver

- existing ODE implementation
- ODE integration

Both solver modes are retained.

```text
FiveNodeModel
      │
      ├─────────────┐
      ▼             ▼
 Gamma Solver    ODE Solver
      │             │
      └──────┬──────┘
             ▼
      SimulationResult
```

The first refactoring must preserve the existing mathematical expressions and evaluation order as closely as possible.

When debugging:

```text
Are BG/Gamma or accumulation calculations correct?
        ↓
Solver
```

---

## 5. Validation — `validation/ValidationRunner`

This component contains the validation workflow currently represented by `runAutoTestNode5Preset()`.

This is an important part of the project and is used to compare the Qt implementation with the reference C implementation.

Responsibilities include the existing validation configuration, such as:

```text
alpha1 = -2.2
alpha2 = selected validation value
alpha3 = 1.2
nu     = 0.70

steps  = 1000
solver = GAMMA
```

and the reference five-node coefficients used by the current validation workflow.

Typical validation values include:

```text
alpha2 = 0
alpha2 = 1
alpha2 = 2
alpha2 = 3
alpha2 = 6
```

Conceptually:

```text
ValidationRunner
       │
       │ validation parameters
       ▼
FiveNodeParameters
       │
       ▼
FiveNodeModel
       │
       ▼
Solver
       │
       ▼
SimulationResult
```

`ValidationRunner` defines the validation experiment.

It should **not implement a second independent copy of the Gamma solver**.

This ensures that normal simulation and reference validation can use the same numerical implementation.

When debugging:

```text
Are the reference validation conditions correct?
        ↓
ValidationRunner
```

---

## 6. Result Output — `output/ResultWriter`

Responsible only for saving calculated results.

Examples:

```text
result.dat
result_stream.csv
result_final.csv
table.txt
parameter/run information
```

Data flow:

```text
Solver
   │
   ▼
SimulationResult
   │
   ▼
ResultWriter
   │
   ├── DAT
   ├── CSV
   └── table
```

`ResultWriter` must not perform numerical simulation.

---

## 7. Plotting — `plot/PlotManager`

Responsible only for visualization of already calculated results.

The plotting component should support multiple output styles instead of forcing all results into one combined figure.

Planned plotting modes include:

### Individual state plots

```text
y1.png
y2.png
y3.png
y4.png
y5.png
```

### Combined five-node plot

```text
y_all.png
```

### Validation/reference-style plots

For example:

```text
alpha2_0.png
alpha2_1.png
alpha2_2.png
alpha2_3.png
alpha2_6.png
```

These plots can be formatted to reproduce the style used in the reference figures for direct comparison.

### Point / parameter-scan plots

The existing alpha2 scan and point-based visualization should also be handled by the plotting component.

Conceptually:

```text
SimulationResult
       │
       ▼
   PlotManager
       │
       ├── individual node plot
       ├── five-node plot
       ├── alpha2/reference plot
       └── point/scan plot
```

`PlotManager` must not calculate the network dynamics.

---

# Debugging Separation

The separated architecture makes it easier to identify the source of a numerical discrepancy.

```text
Problem                         Component
------------------------------------------------
Parameters / coefficients    → FiveNodeParameters
Five-node equations          → FiveNodeModel
Gamma / BG / accumulation    → Solver
ODE calculation              → Solver
Validation configuration     → ValidationRunner
Saved numerical output       → ResultWriter
Visualization                → PlotManager
GUI parameter transfer       → ButtonNetwork
```

This separation is particularly useful for the current comparison between the Qt implementation and the C reference implementation.

---

# Refactoring Strategy

The existing implementation is separated incrementally without intentionally changing the numerical behavior.

```text
FiveNodeParameters
        ↓
FiveNodeModel
        ↓
Solver (Gamma / ODE)
        ↓
ValidationRunner
        ↓
ResultWriter
        ↓
PlotManager
        ↓
ButtonNetwork (GUI)
```

After each major separation step, the numerical output is compared with the original implementation using high-precision output.

```text
Original result.dat
        ↓
17-digit comparison
        ↓
Refactored result.dat
```

After numerical equivalence is confirmed, the GUI and calculation can be connected using Qt signals/slots and moved to a worker thread.

---

# Current Validation

The current validation compares the Qt Gamma implementation with the C reference implementation.

The main focus is the numerical behavior for different `alpha2` values, especially `alpha2 = 3`.

```text
Validation configuration
        ↓
FiveNodeParameters
        ↓
FiveNodeModel
        ↓
Gamma Solver
        ↓
SimulationResult
        ↓
ResultWriter / PlotManager
```

This structure allows the model parameters, equations, numerical solver, output, and visualization to be inspected independently.

---

# Demo / Screenshots

## Network Design & Simulation GUI

<p align="center">
  <img src="docs/Screenshot_20260120_100149_Gallery.jpg" width="700"/>
</p>

## Simulation Output & Plot

<p align="center">
  <img src="docs/Screenshot_20260120_100142_Gallery.jpg" width="700"/>
</p>

---

# Features

- 5-node network (`y₁` to `y₅`)
- Visual GUI node editor
- Custom activation function per connection (`sin`, `tanh`, `relu`)
- ODE and fractional Gamma solvers
- C-reference numerical validation
- Alpha2 parameter scanning
- Result/table display in the GUI
- Result export (`.dat`, `.csv`)
- Gnuplot visualization
