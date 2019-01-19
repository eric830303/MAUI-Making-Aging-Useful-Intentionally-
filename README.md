#MAUI
The program is the implementation of our framework "Making aging useful, intentionally" proposed in DATE

## Outline
* How to run the program?
* Introduction to the arguments in commandline
* Introduction to the parameters in `/setting/Parameters.txt`
* Example of running the program
* How to analyze the experimental result?

## How to run the program?
* The executable is named `maui`

* Make sure that directories `obj/`, `src/`, `setting/` are located at the same path with `maui`.

* Run `make` to compile/link the source/object codes in directories `src/` and `obj/`

* Here is the example of running the program, where the benchmark (i.e., timing report) is `s39418.rpt`

	`./maui -path=onlyff -nonVTA s38417.rpt `

	Note that, the benchmark (e.g., `s38417.rpt`) must be placed at the same path with `maui`

## Introduction to the arguments in commandline
You can see all arguments by running `./maui -h` in the commandline. 

Here, we only introduce commonly-used arguments.

1. `-h`: see the argument.

2. Let our optimization framework only targets the specific groups of aging-critical paths.
	* `-path=onlyff`: Our framework only optimizes the critical paths, starting from FFs to FFs.
	* `-path=pi_ff`: Our framework optimizes the critical paths, starting from FFs to FFs, or starting from PI(primary input) to FFs.
	* `-path=all` (default): Our framework targets all kinds of critical paths.

3. `-nonVTA`: VTA denotes "Vth assignment" for the clock buffers. Our framework incorporates the techniques of DCC deployment and Vth assignment at default, for aging tolerance. Thus, `-nonVTA` forces the framework not to do Vth assignment for clock buffers.

4. `-nodcc`: The argument is similar to the above one `-nonVTA`. `-nondcc` forces the framework not to do DCC deployment.

5. `-dc_for`: Let the framework considers the situations that, the duty cycles, generated from DCCs, are impacted by the aging of DCCs. For example, the duty cycle generated from an 80% DCC will not be exactly 80%, if the aging impact of DCC is considered.
6.  `-CG`: Add clock gating in the framework.


## Introduction to the parameters in `/setting/Parameters.txt`
1. `FIN_CONVERGENT_YEAR`: Used in the aging model. It denotes "finally convergent year".
2. `EXP`: Used in the aging model. It denotes the exponential term in the aging model.
3. `BASE_VTH`: Used in the aging model. 
4. `DC_1_Nf`, `DC_2_Nf` and `DC_3_Nf`: They denote the duty cycles generated from DCCs, if the aging impact of DCC is NOT considered.
5. `DC_1_F`, `DC_2_F` and `DC_3_F`: They denote the duty cycles generated from DCCs, if the aging impact of DCC is considered (i.e., argument`-dc_for`)
6. `LIV_VTH_TECH_OFFSET`: The Vth offset between nominal Vth and high Vth.

## Example of running the program
If you wanna do the experiment with the following conditions:

1. Benchmark: `s38417.rpt`
2. Optimized group of aging-critical paths: The critical paths from FF to FF (i.e., `-path=onlyff`)
3. Do not apply the technique of Vth assignment for aging tolerance. (i.e., `-noVTA`)
4. Consider the aging impact of DCC on the duty cycle. (i.e., `-dc_for`)

Hence, the corresponding command is as follows:

`./maui -path=pi_ff -nonVTA -dc_for s38417.rpt`

Then, if you wanna apply the technique of Vth assignment in the framework. The corresponding command:

`./maui -path=pi_ff -dc_for s38417.rpt`

Further, if you wanna see the influence of clock gating (i.e., `-CG`) on the experimental result. The corresponding command:
`./maui -path=pi_ff -dc_for -CG s38417.rpt`

The results will be shown on the screen of your terminal. Moreover, the program will automatically generate a directory, named with `s38417_output`, which contains the files recording CNF clauses, resulting DCC deployments and leader selections.


## How to analyze the expeimental results
To be continued...

