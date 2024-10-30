EPANET

Primeira versão 2.00.12 - 02/2008
Versão 2.1 - 07/2016
Versão 2.2 - 08/2018
Versão 2.3 - 02/2020

Costa usou versão 2.0

-------------------------------------------------------

Demanda em m³/h. Valores entre 22.71247 e 227.1247!!!

Vazão máxima das bombas: 1816.9976 m³/h
[CURVES]
;ID              	X-Value     	Y-Value
;PUMP: PUMP:     
 1               	0           	91.44       
 1               	454.2494    	89.0016     
 1               	908.4988    	82.296      
 1               	1362.7482   	70.104      
 1               	1816.9976   	55.1688     

X-Value: Representa a vazão da bomba (m³/h).
Y-Value: Representa a altura manométrica da bomba (m).
Vazão calculada dinâmicamente a partir da relação desses dois valores.

A solução ótima global do paper do Costa é de 121211110022222121000210, sendo os valores a quantidade de bombas ligadas naquela hora.

-------------------------------------------------------

EPANET 2.2 USER'S MANUAL

1.4 Steps in Using EPANET
One typically carries out the following steps when using EPANET to model a water distribution system:
1. Draw a network representation of your distribution system (see Section 6.2) or import a basic description of the
network placed in a text file (see Section 11.4).
2. Edit the properties of the objects that make up the system (see Section 6.4).
3. Describe how the system is operated (see Section 6.5).
4. Select a set of analysis options (see Section 8.1).
5. Run a hydraulic/water quality analysis (see Section 8.2).
6. View the results of the analysis (see Section 9).