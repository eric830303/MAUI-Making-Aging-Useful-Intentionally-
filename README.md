# MAUI
The program is the implementation of our framework "Making aging useful, intentionally" proposed in DATE

## Contact
1. Name: Tien-Hung, Tseng
2. Email: eric830303.cs05g@g2.nctu.edu.tw 

## Outline
* [Files and Directories] (#1)
* [Introduction to the arguments in commandline] (#2)
* [Introduction to the parameters in `/setting/Parameters.txt`] (#3)
* [Example of running the program] (#4)
* [Process variations] (#5)
* [Aging model] (#6)

## <h2 name="1"> Files and Directories </h1>
* The executable is named `maui`

* The 2 executables of SAT solver are located in the dir `MINISATs/`. One can be run on OSX, the other can be run on Linux.

* You can ignore the existence of the directory `gnuplot/`, which contains the PVs (process variations) results of the benchmarks, after applying our framework. We upload the directory here, because there exist some scripts of gnuplot to generate the figures.

* Make sure that directories `obj/`, `src/`, `setting/` are located at the same path with `maui`. The directory `obj/` does not exist here/github. You must create it youself by linux command `mkdir obj`.

* Run `make` to compile/link the source/object codes in directories `src/` and `obj/`. Also, while you are running `make`, the script will automatically detect the current platform (i.e., operating system), and put the proper executable of SAT solver from the `MINISATs/` to the current dir.

* Run `make clean` to remove object codes in `obj/` 

* Our program input is the timing report of the benchmark, e.g., `s38417.rpt`. I do not upload them here due to large file size. Please contact me if you need them.

* The main output of our program is a directory, which contains the CNF clauses from iteration to iteration of binary search, and DCC deployment and high-Vth leader selection.


## <h2 name="2"> Introduction to the arguments in commandline</h2>
You can see all arguments by running `./maui -h` in the commandline. 

Here, we only introduce commonly-used arguments.

1. `-h`: see the argument.

2. Let our optimization framework only targets the specific groups of aging-critical paths.
	* `-path=onlyff`: Our framework only optimizes the critical paths, starting from FFs to FFs.
	* `-path=pi_ff`: Our framework optimizes the critical paths, starting from FFs to FFs, and starting from PI(primary input) to FFs.
	* `-path=all` (default): Our framework targets all groups of critical paths.

3. `-nonVTA`: VTA denotes "Vth assignment" for the clock buffers. Our framework incorporates the techniques of DCC deployment and Vth assignment at default, for aging tolerance. Thus, `-nonVTA` forces the framework not to do Vth assignment for clock buffers.

4. `-nodcc`: The argument is similar to the above one `-nonVTA`. `-nodcc` forces the framework not to do DCC deployment.

5. `-dc_for`: Let the framework consider the situations that, the duty cycles, generated from DCCs, are impacted by the aging of DCCs. For example, the duty cycle generated from an 80% DCC will not be exactly 80%, if the aging impact of DCC is considered.
6.  `-CG`: Add clock gating in the framework.


## <h2 name="3"> Introduction to the parameters in `/setting/Parameters.txt` </h3>
1. `FIN_CONVERGENT_YEAR`: Used in the aging model. It denotes "finally convergent year".
2. `EXP`: Used in the aging model. It denotes the exponential term in the aging model.
3. `BASE_VTH`: Used in the aging model. 
4. `DC_1_Nf`, `DC_2_Nf` and `DC_3_Nf`: They denote the duty cycles generated from DCCs, if the aging impact of DCC is NOT considered.
5. `DC_1_F`, `DC_2_F` and `DC_3_F`: They denote the duty cycles generated from DCCs, if the aging impact of DCC is considered (i.e., argument`-dc_for`)
6. `LIV_VTH_TECH_OFFSET`: The Vth offset between nominal Vth and high Vth.

## <h2 name="4"> Example of running the program </h4>
If you wanna do the experiment with the following conditions:

1. Benchmark: `s38417.rpt`
2. Optimized group of aging-critical paths: The critical paths from FF to FF (i.e., `-path=onlyff`)
3. Do not apply the technique of Vth assignment for aging tolerance. (i.e., `-noVTA`)
4. Consider the aging impact of DCC on the duty cycle. (i.e., `-dc_for`)

Hence, the corresponding command is as follows:

`./maui -path=onlyff -nonVTA -dc_for s38417.rpt`

Then, if you wanna apply the technique of Vth assignment in the framework. The corresponding command:

`./maui -path=onlyff -dc_for s38417.rpt`

Further, if you wanna see the influence of clock gating (i.e., `-CG`) on the experimental result. The corresponding command:
`./maui -path=onlyff -dc_for -CG s38417.rpt`

The results will be shown on the screen of your terminal. Moreover, the program will automatically generate a directory, named with `s38417_output`, which contains the files recording CNF clauses, resulting DCC deployments and leader selections.



## <h2 name="5"> Process Variations </h5>
In the section, we focuss on analyzing the impact of PVs (process variations) on the aging tolerance.

Let's see an example.

After applying the our framework on `s38417.rpt` with the following cmd:

`./maui -path=onlyff s38417.rpt`

A directory, named with `s38417_output/` is generated.

<img src="./readme_fig/dir.jpg" width="400" height="220" />

In the directory, the dcc deployments and leader selections are recoreded in the file `s38417_output/DccVTA_0.901800.txt`, where the last few characters `0.901800` represents the optimized clock period of the benchmark `s38417`.

<img src="./readme_fig/files_in_dir.jpg" width="400" height="250" />

Then, please copy the content in `s38417_output/DccVTA_0.901800.txt` to `setting/DccVTA.txt`, which the analysis utility of our program will read at default.

<img src="./readme_fig/copy.jpg" width="530" height="40" />


Then, do PV analysis with the following cmd:
`./maui -path=onlyff -PV s38417.rpt`

Then, there are some interaction between the user and the program, by some interactive inputs! (i.e, the count of Monte-Carlo instances, fresh Tc..etc)

<img src="./readme_fig/PV_type.jpg" width="400" height="200" />

The Tc\_fresh and Tc\_aged can be refered to the experimental results.

<img src="./readme_fig/Tc.jpg" width="320" height="200" />


The output of the PV analysis utility is a file, named with `Imp_dist.txt`, meaning "improvement distribution"

<img src="./readme_fig/Imp_dist.jpg" width="400" height="240" />

Then, you can use the gnuplot to plot the improment distributions of Monte-Carlo instances. The template of the gnuplot script (*.gp) can be refered to the dir `gnuplot/`

## <h2 name="6"> Aging model </h6>
The aging model can be refered...



