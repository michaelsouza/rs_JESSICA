# Rctart (v1.0.0)

## Description

The main features of the class are as follows.

* The class document and custom packages are available in a *single* folder.
* Compatible with external editors.
* Stix2 is default font for clear text.
* It is possible to change fonts using the class options 'timesnews=on' or 'latinmodern=on'
* Custom environments for notes and information.
* Custom colours when code is inserted for programming languages (Matlab, C, C++, and LaTeX).
* Appropriate conjunction ('y' for Spanish, 'and' for English, 'e' for Portuguese) when two authors are included.
* Easy to customise with boolean functions and with class options.
* Special design for unnumbered sections.

## Updates Log

**Version 1.0.0 (22/06/2024)**

Launch of the first version of rctart class, made especially for academic articles for Recet journal and laboratory reports.
This class is based on Rho class. Thanks for Rho class team.

*Document Fonts*
[1] The default fonts are setting to be stix2, helvet and lmtt.
[2] The class option 'timesnews=on' changes fonts to newtxtext, newtxmath, helvet and lmtt. newtxmath could be more appreciated for some users.
[3] The class option 'latinmodern=on' changes fonts to lmodern, lmss and lmtt. Some users can test the resulting document.

*Title style*
[4] Introducing a new front cover to Recet Article.
[5] Automatic adjustments if the followings commands are not declared in the preamble:
    - corres, doi, received, reviewed, accepted, published, license, eissn, articlenum
    - theyear, thevolume, thelocal, themonths, theyears
    - journalname
    - dates
    
*Abstract*
[6] Now it is possible to type Portuguese, Spanish, English abstracts with automatic title and keywords:
    - keywords
[7] New command prints the keywords:
    - printkeywords{key1, key2, key3, ...}
    - printkeywords{\@keywords}
[8] No more needs to load Abstract package. Abstract environment is constructed with \renewtcolorbox. Tcolorbox 'most' option is loaded and 'breakable' is used in abstract. It can have it's color back colored. New class option:
    -  If the color abstract is enable:'colorabst=on'
    
*Article information options*
[8] New command '\setbool{article-info}{t/f}'  to disable or enable the article information to be printed (main.tex).
    - If the article information is enable: '\setbool{article-info}{true}'
    - If the article information is disable: '\setbool{article-info}{false}'

*Line numbering options*
[10] New class option 'linenumbers=off' and 'linenumbers=on' are present to disable or enable line numbering (main.tex).
    - If the line numbering is enable: 'linenumbers=on'
    - If the line numbering is disable: 'linenumbers=off'

*Header and footer information options*
[11] Automatic adjustments if the followings commands are not declared in the preamble:
    - leadauthor
    - footinfo
    - smalltitle
    - institution
    - theday

*Rctartenvs (v1.0.0)*
[12] Enviroments was imported from Rho class: note, info, rctartenv. Environments have a small new design. The border is the same color as the background for a cleaner look.
[13] A new environment citacao was introduced. It types citation references based on the same environment as typed in abntex2cite package.

*Rctartbabel (v1.0.0)*
[14] We introduced a new package 'rctartbabel' based on 'rhobabel' for translations and language of the document.
[15] New command '\setboolean{es-babel}{t/f}' to change the language to spanish:
    - If spanish babel is enable: '\setboolean{es-babel}{true}'
    - If spanish babel is disable: '\setboolean{es-babel}{false}'
[16] Rctartbabel is supporting Portuguese, English and Spanish by the way of '\addto\captions'language''. Making que maintenace more easy.
[17] If you would like to write in another language, you can modify this package to your needs.

## Supporting

If you want to acknowledge rctart class, adding a sentence mentioning that 'this report/article was typeset with the rctart class' would be great!

## License

This work is licensed under Creative Commons CC BY 4.0. 
To view a copy of CC BY 4.0 DEED, visit:

    https://creativecommons.org/licenses/by/4.0/

This work consists of all files listed below as well as the products of their compilation.

```
rctart/
`-- rctart-class/
    |-- rctart.cls
    |-- rctartbabel.sty
    |-- rctartenvs.sty
`-- main.tex
`-- rct.bib
```

## Contact us

Do you like the design, but you found a bug? Is there something you would have done differently? Any contribution is welcome!

*Email: silvio.granja@unemat.br

---
Enjoy writing with rctart class :D
