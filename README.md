This is used to create containers for collecting & processing inputs/outputs of
FireSTARR. The current workflow is to either run the containers locally or in Azure,
and (optionally) publish the results to a geoserver WMS.

For performance reasons, the version of FireSTARR used right now is stripped down to just
the code required to create the burn probability maps that are published, and it does
not contain many of the options from the version at https://github.com/CWFMF/FireSTARR/tree/575f6c064b12eae8716a221d25343398073f0c97z.

Eventually things will be re-implemented in the submodule repo at
https://github.com/cwmfmf/firestarr-cpp.

# About FireSTARR

## Overview

FireSTARR is designed to support wildland fire response decision-making, and is the fire growth algorithm originally in the FireGUARD suite of programs.

It focuses on the generation of burn probabilities from replicated simulation of fire growth, smouldering, and natural extinction under the weather and stochastic fire behaviour scenarios.

## Additional License Condition

All covered works (e.g., copies of this work, derived works) must include a copy of the file (or an updated version of it) that details credits for work up to the time of the original open source release. That file is available [here](./ORIGIN.md).

## Publications

FireGUARD is published at the following locations:

- [Weather forecast model](https://doi.org/10.3390/fire3020016)
- Burn probability model (In progress)
- [Impact and likelihood-weighted impact model](https://doi.org/10.1071/WF18189)
