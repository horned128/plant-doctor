# Plant Doctor workspace

`PlantDoctor/` is the LEXIDE-Ω managed-build project for the
DT-EBML63Q2557 (ML63Q2557). Import that directory as an existing project;
do not select this workspace container itself as the Eclipse project, and do
not enable **Copy projects into workspace** during import.

The project links `../CommonFiles` through a portable Eclipse linked resource.
The repository root must therefore keep this layout:

```text
CommonFiles/
PlantDoctorWorkspace/
  PlantDoctor/
```
