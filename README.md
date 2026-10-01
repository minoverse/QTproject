# Hopfield Fractional Network Simulator

A Qt-based GUI application for building, simulating, analyzing, and validating
a five-node Hopfield neural network.

The project supports:

- Ordinary Differential Equation (ODE) solver
- Fractional-order Gamma-based solver
- Interactive five-node network configuration
- Custom connection weights and activation functions
- Alpha2 parameter scanning
- Numerical validation against the original C reference implementation
- Result export and Gnuplot visualization

---

# Architecture

The project is separated into independent components for GUI handling,
mathematical model definition, numerical computation, parameter analysis,
validation, result output, and plotting.

```text
                    ┌────────────────────────┐
                    │     ButtonNetwork      │
                    │                        │
                    │ GUI / orchestration    │
                    │ network editing        │
                    │ user interaction       │
                    └───────────┬────────────┘
                                │
          ┌─────────────────────┼─────────────────────┐
          │                     │                     │
          ▼                     ▼                     ▼
┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐
│ FiveNodeParameters│  │  FiveNodeModel   │  │  Alpha2Scanner   │
│                  │  │                  │  │                  │
│ simulation       │  │ mathematical     │  │ parameter sweep  │
│ parameters       │  │ equations        │  │ scan data        │
└────────┬─────────┘  └────────┬─────────┘  └────────┬─────────┘
         │                     │                     │
         └─────────────┬───────┘                     │
                       ▼                             │
              ┌──────────────────┐                   │
              │      Solver      │◄──────────────────┘
              │                  │
              │ ODE solver       │
              │ Gamma solver     │
              │ Gamma/BG weights │
              └────────┬─────────┘
                       │
                       ▼
                Numerical Results
                       │
          ┌────────────┼─────────────┐
          │            │             │
          ▼            ▼             ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────────┐
│ ResultWriter │ │ PlotManager  │ │ ValidationRunner │
│              │ │              │ │                  │
│ DAT / CSV    │ │ Gnuplot      │ │ C-reference      │
│ tables       │ │ PNG output   │ │ comparison       │
└──────────────┘ └──────────────┘ └──────────────────┘
```

The main goal of this separation is to keep the numerical implementation
independent from GUI, plotting, and file-output responsibilities.

The refactoring preserves the existing numerical equations, parameters,
Gamma/ODE calculations, Gamma-weight calculation, and calculation order.

Numerical behavior is checked against the original C implementation.

---

# Directory Structure

```text
ButtonNetwork/
│
├── main.cpp
├── buttonnetwork.cpp
├── buttonnetwork.h
├── ButtonNetwork.pro
├── README.md
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
├── analysis/
│   ├── alpha2scanner.h
│   └── alpha2scanner.cpp
│
├── validation/
│   ├── main_validation.cpp
│   ├── validationrunner.h
│   ├── validationrunner.cpp
│   └── reference/
│       ├── result_a2_0.000000.dat
│       ├── result_a2_1.000000.dat
│       ├── result_a2_2.000000.dat
│       └── result_a2_3.000000.dat
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

## 1. GUI / Orchestration — `ButtonNetwork`

`ButtonNetwork` handles user interaction and coordinates the separated
simulation components.

Main responsibilities:

- create and edit network nodes
- create and edit connections
- handle mouse interaction
- draw the network
- collect GUI parameters
- request numerical simulations
- display results
- request Alpha2 scans
- request plotting
- start validation/demo workflows

The numerical Gamma and ODE algorithms are not implemented in the GUI.

```text
User
  │
  ▼
ButtonNetwork
  │
  ├── parameters
  ├── connections
  └── simulation request
          │
          ▼
     Model + Solver
```

This keeps GUI-specific behavior separate from the numerical implementation.

---

## 2. Parameters — `model/FiveNodeParameters`

`FiveNodeParameters` stores the numerical configuration of the five-node
system.

The main parameters include:

```text
alpha1
alpha2
alpha3
nu
tMax
solverMode
```

It also contains configuration for the node-specific gate functions.

For example:

```text
GateNode4
GateNode5
```

Separating the parameters from the solver makes it easier to determine
whether a discrepancy originates from the simulation configuration or from
the numerical algorithm itself.

---

## 3. Mathematical Model — `model/FiveNodeModel`

`FiveNodeModel` contains the mathematical definition of the five-node
network.

Responsibilities include:

- evaluation of the five-node equations
- activation functions
- connection-dependent function evaluation
- node-specific gate behavior
- mathematical right-hand-side evaluation

Conceptually:

```text
state + parameters + weights
             │
             ▼
      FiveNodeModel
             │
             ▼
       model evaluation
```

The model describes **what mathematical system is evaluated**.

It does not:

- create GUI widgets
- generate plots
- write result files
- implement the Gamma history accumulation loop

---

## 4. Numerical Solver — `solver/Solver`

`Solver` contains the numerical algorithms used to evolve the five-node
system.

Two solver modes are supported.

### ODE Solver

The ODE path performs the existing ordinary differential equation
integration.

```text
FiveNodeModel
      │
      ▼
  ODE Solver
      │
      ▼
Numerical trajectory
```

### Fractional Gamma Solver

The Gamma solver performs the fractional-order numerical calculation.

Its responsibilities include:

- Gamma/Beta-related weight calculation
- BG/Gamma weight calculation
- history accumulation
- `om` iteration
- `r` iteration
- fractional contribution accumulation
- initial-value contribution
- time-step evolution

The Gamma weight calculation is kept inside the solver:

```cpp
double gammaWeight(int om, int r, double nu) const;
```

Conceptually:

```text
                    previous states
                          │
                          ▼
                   Gamma/BG weights
                          │
                          ▼
                   history accumulation
                          │
                          ▼
                   model evaluation
                          │
                          ▼
                     next state
```

This part is especially important because the Gamma/BG weighting and the
order of the fractional accumulation directly affect the numerical result.

The refactoring therefore keeps the numerical behavior of this calculation
consistent with the reference implementation.

---

# Alpha2 Parameter Analysis

## `analysis/Alpha2Scanner`

`Alpha2Scanner` performs the Alpha2 parameter-sweep calculation independently
from the GUI.

Responsibilities:

- iterate over the configured Alpha2 range
- temporarily apply each Alpha2 value
- invoke the selected numerical solver
- discard the configured transient region
- sample the resulting trajectories
- generate 2D scan data
- generate 3D scan data

The generated files include:

```text
alpha2_scan_2d.dat
alpha2_scan_3d.dat
```

Conceptually:

```text
Alpha2 range
     │
     ▼
Alpha2Scanner
     │
     ├── alpha2 = a0 → Solver
     ├── alpha2 = a1 → Solver
     ├── alpha2 = a2 → Solver
     └── ...
             │
             ▼
       sampled states
             │
             ▼
 alpha2_scan_2d.dat
 alpha2_scan_3d.dat
```

`Alpha2Scanner` does not generate plots.

After the scan data is generated, visualization is delegated to
`PlotManager`.

---

# Validation

## `validation/ValidationRunner`

`ValidationRunner` defines the reference validation experiment.

The validation uses the same `FiveNodeModel` and `Solver` used by the normal
application.

It does **not** contain a second independent implementation of the Gamma
solver.

The reference preset uses:

```text
alpha1 = -2.2
alpha2 = selected validation value
alpha3 = 1.2
nu     = 0.70

tMax   = 1000
solver = GAMMA
```

The reference network connections are:

```text
1 -> 4   -0.6   sin_exp
4 -> 1    0.7   tanh

1 -> 3   -0.8   sin_exp
3 -> 1    1.7   tanh

2 -> 3    2.0   sin_exp
3 -> 2   -0.4   tanh

1 -> 2   -0.3   tanh
2 -> 1   -3.0   sin_exp

2 -> 5    0.4   sin_exp
5 -> 2    1.7   tanh

3 -> 3    3.0   sin_exp
```

The validation workflow is:

```text
Reference parameters
        │
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
Qt numerical trajectory
        │
        ▼
ValidationRunner
        │
        ├─────────────── reference .dat
        │
        ▼
high-precision comparison
```

This makes it possible to test the production numerical implementation
directly instead of maintaining a separate validation solver.

---

# Numerical Validation Results

The refactored Gamma solver was compared against output generated by the
original C reference implementation.

Four Alpha2 cases are currently included in the automated validation:

| Alpha2 | Samples | Nodes | Result |
|-------:|--------:|------:|:------:|
| 0.0 | 1001 | 5 | PASS |
| 1.0 | 1001 | 5 | PASS |
| 2.0 | 1001 | 5 | PASS |
| 3.0 | 1001 | 5 | PASS |

Validation output:

```text
=== Numerical Validation ===

alpha2 = 0.0 : PASS
   Exact match: 1001 rows x 5 nodes

alpha2 = 1.0 : PASS
   Exact match: 1001 rows x 5 nodes

alpha2 = 2.0 : PASS
   Exact match: 1001 rows x 5 nodes

alpha2 = 3.0 : PASS
   Exact match: 1001 rows x 5 nodes

Overall: PASS
```

For these four validation cases, the refactored Qt Gamma implementation
produces the same numerical trajectory as the generated C-reference data.

This validation is particularly important for confirming that separation of:

```text
model
solver
validation
output
plotting
analysis
GUI
```

did not alter the Gamma solver calculation.

---

# Result Output

## `output/ResultWriter`

`ResultWriter` handles persistence of already calculated numerical results.

Typical output files include:

```text
result.dat
result_stream.csv
result_final.csv
table.txt
params.txt
run_info.txt
```

For a normal simulation:

```text
Solver
   │
   ▼
Numerical trajectory
   │
   ▼
ResultWriter
   │
   ├── result.dat
   ├── result_stream.csv
   ├── result_final.csv
   └── table.txt
```

`ResultWriter` does not calculate the network dynamics.

---

# Plotting

## `plot/PlotManager`

`PlotManager` is responsible for visualization of already calculated data.

It creates Gnuplot scripts and executes Gnuplot independently from the
numerical solver.

### Time-series plots

A simulation generates a combined plot:

```text
y_all.png
```

containing all five state trajectories.

It also generates separate plots:

```text
y1.png
y2.png
y3.png
y4.png
y5.png
```

This allows each state trajectory to be inspected independently while
retaining the combined five-node visualization.

### Alpha2 scan plots

Alpha2 scanning generates:

```text
alpha2_y1.png
alpha2_y2.png
alpha2_y3.png
alpha2_y4.png
alpha2_y5.png
```

using the data produced by `Alpha2Scanner`.

The separation is:

```text
Alpha2Scanner
      │
      ▼
scan .dat files
      │
      ▼
 PlotManager
      │
      ▼
scan PNG files
```

`PlotManager` does not calculate network dynamics.

---



# Debugging Separation

The architecture is designed so that different classes correspond to
different categories of problems.

```text
Problem                              Component
---------------------------------------------------------
Simulation parameters              FiveNodeParameters
Five-node equations                FiveNodeModel
Gamma/BG weights                   Solver
Fractional accumulation            Solver
ODE calculation                    Solver
Alpha2 parameter sweep             Alpha2Scanner
Reference validation conditions    ValidationRunner
Saved numerical output             ResultWriter
Visualization / Gnuplot            PlotManager
GUI parameter transfer             ButtonNetwork
```

For example, if a numerical discrepancy appears:

```text
Reference mismatch
       │
       ▼
Are parameters identical?
       │
       ├── No  → FiveNodeParameters / ValidationRunner
       │
       ▼
Are equations identical?
       │
       ├── No  → FiveNodeModel
       │
       ▼
Are Gamma/BG weights identical?
       │
       ├── No  → Solver
       │
       ▼
Is fractional accumulation identical?
       │
       ├── No  → Solver
       │
       ▼
Check output precision / comparison
```

This separation is particularly useful when comparing the Qt implementation
against the original C implementation.

---



# Building

The project uses Qt 6, qmake, GSL, and Gnuplot.

Typical Ubuntu/Debian dependencies include:

```bash
sudo apt install qt6-base-dev libgsl-dev gnuplot
```

Build the GUI application:

```bash
cd ~/ButtonNetwork

qmake6 ButtonNetwork.pro
make -j$(nproc)
```

Run:

```bash
./ButtonNetwork
```

---

# Numerical Validation Build

The standalone validation executable can be built independently from the
GUI.

For example:

```bash
cd validation/build
make -j$(nproc)

cd ../..
./validation/build/validate_solver
```

Expected result:

```text
=== Numerical Validation ===

alpha2 = 0.0 : PASS
   Exact match: 1001 rows x 5 nodes

alpha2 = 1.0 : PASS
   Exact match: 1001 rows x 5 nodes

alpha2 = 2.0 : PASS
   Exact match: 1001 rows x 5 nodes

alpha2 = 3.0 : PASS
   Exact match: 1001 rows x 5 nodes

Overall: PASS
```

A non-zero validation exit code indicates that at least one comparison
failed.

---

# Features

- Five-node Hopfield network
- Qt graphical network editor
- Editable weighted connections
- Connection-dependent activation functions
- Ordinary differential equation solver
- Fractional Gamma-based solver
- Gamma/BG history weighting
- Alpha2 parameter scanning
- Standalone numerical validation
- C-reference comparison
- High-precision result comparison
- DAT output
- CSV output
- table output
- combined five-node plots
- individual `y1`–`y5` plots
- individual Alpha2 scan plots
- per-run result directories


This provides a reproducible check that the current Gamma solver remains
consistent with the original C reference implementation for the validated
parameter sets.
