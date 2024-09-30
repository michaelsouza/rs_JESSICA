%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% --------------------------------------------------------
% Rctart
% LaTeX Class
% Version 1.0.0 (21/05/2024)
%
% Authors: 
% Silvio Cesar Garcia Granja (silvio.granja@unemat.br)
% 
% Baseado na classe rho de autoria de:
% Guillermo Jimenez (memo.notess1@gmail.com)
% Eduardo Gracidas (eduardo.gracidas29@gmail.com)
% --------------------------------------------------------
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% --------------------------------------------------------
% You may modify 'rctart.cls' file according to your
% needs and preferences. This LaTeX class file defines 
% the document layout and behavior.
% --------------------------------------------------------
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% --------------------------------------------------------
% 						 WARNING!
% --------------------------------------------------------
% It is important to proceed with caution and ensure that 
% any modifications made are compatible with LaTeX 
% syntax and conventions to avoid errors or unexpected 
% behavior in the document compilation process.
% --------------------------------------------------------
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%----------------------------------------------------------
% CLASS CONFIGURATION
%----------------------------------------------------------

\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{rctart-class/rctart}[2024/05/21 Rctart LaTeX class]

\RequirePackage{xifthen}

\newboolean{ShowLinenumbers}
\newboolean{UseLatinmodern}
\newboolean{UseTimes}
\newboolean{semanafacetclean}
\newboolean{article-info}
\newboolean{tarja-info}
\newboolean{articleinfotop}
\newboolean{colorabst}
\newboolean{printhead}
\newboolean{printfoot}

\setboolean{ShowLinenumbers}{true}
\setboolean{UseLatinmodern}{false}
\setboolean{UseTimes}{false}
\setboolean{semanafacetclean}{false}
\setboolean{article-info}{true}
\setboolean{tarja-info}{true}
\setboolean{articleinfotop}{true} % true para topo, false para baixo do título
\setboolean{colorabst}{false}
\setboolean{printhead}{true}
\setboolean{printfoot}{true}



\DeclareOption{linenumbers=on}{\setboolean{ShowLinenumbers}{true}}
\DeclareOption{linenumbers=off}{\setboolean{ShowLinenumbers}{false}}
\DeclareOption{latinmodern=on}{\setboolean{UseLatinmodern}{true}}
\DeclareOption{timesnews=on}{\setboolean{UseTimes}{true}}
% \DeclareOption{colorabst=on}{\setboolean{colorabst}{true}}
\DeclareOption{semanafacetclean}{
    \setboolean{article-info}{false}              % false to hide the article-info section
    \setboolean{tarja-info}{false}                % false to hide the article-info section
    \setboolean{articleinfotop}{false}            % false to print article-info after affiliations
    \setboolean{colorabst}{false}                 % colore o resumo e abstract
    \setboolean{printhead}{false}
    \setboolean{printfoot}{false}
}

\DeclareOption*{\PassOptionsToClass{\CurrentOption}{extarticle}}
\ProcessOptions\relax
\LoadClass[twocolumn]{extarticle}
\AtEndOfClass{\RequirePackage{microtype}}

%----------------------------------------------------------
% REQUIRED PACKAGES
%----------------------------------------------------------

\RequirePackage[utf8]{inputenc}
\RequirePackage{etoolbox}
\RequirePackage[framemethod=tikz]{mdframed}
\RequirePackage[most]{tcolorbox}
\RequirePackage{titling}
\RequirePackage{lettrine}
\RequirePackage[switch]{lineno}
\ifthenelse{\boolean{ShowLinenumbers}}
{}%
{ \renewcommand{\linenumbers}{}
  \renewcommand{\nolinenumbers}{}%
}

\RequirePackage{microtype}
\RequirePackage[bottom,hang,flushmargin,ragged]{footmisc}
\RequirePackage{fancyhdr}
\RequirePackage{adjustbox}
\RequirePackage{adforn}
\RequirePackage{lastpage}
\RequirePackage[explicit]{titlesec}
\RequirePackage{booktabs}
\RequirePackage{array} 
\RequirePackage{setspace}
\RequirePackage{iflang}
\RequirePackage{listings}
\RequirePackage{lipsum}
\RequirePackage{fontawesome5}       % For icons
\RequirePackage{chemfig}            % Chemical structures
\RequirePackage{circuitikz}         % Circuits schematics
\RequirePackage{supertabular}
\RequirePackage{matlab-prettifier}
\RequirePackage{listings}
\RequirePackage{csquotes}
\RequirePackage{ragged2e}
\RequirePackage{ccicons}
\RequirePackage{imakeidx}
\RequirePackage{subcaption}
\RequirePackage{stfloats} 
\RequirePackage{authblk}
\RequirePackage{caption}
\RequirePackage{multicol}
\RequirePackage{setspace}
\RequirePackage{icomma}
\RequirePackage{indentfirst}
% \RequirePackage{balance}

%----------------------------------------------------------
% RCTART CUSTOM PACKAGES (location/name)
%----------------------------------------------------------

\RequirePackage{rctart-class/rctartbabel}
\RequirePackage{rctart-class/rctartenvs}

%----------------------------------------------------------
% LINE NUMBERS-, CORRESPONDING AUTHOR SECTION-, ABTRACT-, NEWBOOL
%----------------------------------------------------------

\newbool{linenumbers}

\newbool{rctart-abstract}



%----------------------------------------------------------
% PAGE LAYOUT
%----------------------------------------------------------

\RequirePackage{geometry}

\if@twocolumn%
  \geometry{
    includeheadfoot,
    left=1.27cm,%1.91cm, 
    right=1.27cm,%1.91cm, 
    top=0.5cm, 
    bottom=1.0cm,%1.27cm, 
    headsep=0.50cm,
    headheight=1.0cm,
    footskip=1.0cm,
}%
\setlength{\columnsep}{15pt}
\else% \@twocolumnfalse
  \geometry{
    includeheadfoot,
    left=2.00cm,%1.91cm, 
    right=2.00cm,%1.91cm, 
    top=2cm, 
    bottom=2cm, 
    headsep=0.75cm
}%%
\fi

%----------------------------------------------------------

\RequirePackage{graphicx}
\RequirePackage{here}

%----------------------------------------------------------
% PACKAGES FOR BOXES
%----------------------------------------------------------

\RequirePackage{adjustbox}
\RequirePackage{colortbl}


%----------------------------------------------------------
% MATH PACKAGES
%----------------------------------------------------------

%!TEX In case of using another font that is not stix2 uncomment 'amssymb'

\RequirePackage{amsmath}
\RequirePackage{amsfonts}
\RequirePackage{mathtools}
% \RequirePackage{amssymb}

% Equation skip value
\newlength{\eqskip}\setlength{\eqskip}{6pt}%{8pt}
\expandafter\def\expandafter\normalsize\expandafter{%
    \normalsize%
    \setlength\abovedisplayskip{\eqskip}%
    \setlength\belowdisplayskip{\eqskip}%
    \setlength\abovedisplayshortskip{\eqskip-\baselineskip}%
    \setlength\belowdisplayshortskip{\eqskip}%
}

%----------------------------------------------------------
% FONTS
%----------------------------------------------------------

% \usepackage[notextcomp]{stix2}
% \RequirePackage[scaled]{helvet}
% \renewcommand{\ttdefault}{lmtt}


\ifthenelse{\boolean{UseLatinmodern}}
{%
    \RequirePackage{lmodern}
    \renewcommand{\sfdefault}{lmss}
    \renewcommand{\ttdefault}{lmtt}
    \RequirePackage{amssymb}
}%
{%
    \ifthenelse{\boolean{UseTimes}}
    {%
%         \RequirePackage{mathptmx}
        \RequirePackage{newtxtext} 		% Usa a fonte times para texto
        \RequirePackage{newtxmath}      % symbol para matemática
        \RequirePackage[scaled=0.90]{helvet} 	% Usa a fonte Helvetica para texto sem serifa (ss)
%         \RequirePackage{courier} 		% Usa a fonte Courier para texto 
                                        % monoespaçado (tt)
        \renewcommand{\ttdefault}{lmtt}  % Usa a fonte latinmodern tt para texto monoespaçado (tt)
    }{%
        \RequirePackage[notextcomp]{stix2}
        \RequirePackage[scaled=0.90]{helvet}  % Usa a fonte Helvetica para texto sem serifa (ss)
        \renewcommand{\ttdefault}{lmtt}  % Usa a fonte latinmodern tt para texto monoespaçado (tt)
    }
}


%----------------------------------------------------------
% URLs STYLE
%----------------------------------------------------------

\RequirePackage{url}
\RequirePackage{xurl}
\renewcommand\UrlFont{\selectfont}

%----------------------------------------------------------

\RequirePackage[colorlinks=true, allcolors=rctartcolor]{hyperref}
\RequirePackage{cleveref}
\RequirePackage{bookmark}

%----------------------------------------------------------
% ITEMS
%----------------------------------------------------------

\RequirePackage{enumitem}
\setlist{noitemsep}

%----------------------------------------------------------
% FIRST PAGE-, HEADER AND FOOTER
%----------------------------------------------------------

% New commands
\newcommand{\footerfont}{\normalfont\sffamily\scriptsize}%\fontsize{7}{9}\selectfont}
\newcommand{\institution}[1]{\def\@institution{#1}}
\newcommand{\leadauthor}[1]{\def\@leadauthor{#1}}
\newcommand{\footinfo}[1]{\def\@footinfo{#1}}
\newcommand{\smalltitle}[1]{\def\@smalltitle{#1}}
\newcommand{\theday}[1]{\def\@theday{#1}}
\newcommand{\theyear}[1]{\def\@theyear{#1}}
\newcommand{\thevolume}[1]{\def\@thevolume{#1}}

\newcommand{\thelocal}[1]{\def\@thelocal{#1}}
\newcommand{\themonths}[1]{\def\@themonths{#1}}
\newcommand{\theyears}[1]{\def\@theyears{#1}}

\theday{\@thelocal, v.\@thevolume, \@themonths\space\@theyears}


\pagestyle{fancy}
\pagenumbering{arabic}


% First page style
\fancypagestyle{firststyle}{
    \renewcommand{\headrulewidth}{0pt} % No header rule
    \renewcommand{\footrulewidth}{0pt} % No footer rule
    
    \fancyfoot[R]{\ifbool{printfoot}{\footerfont\thepage\textendash\pageref{LastPage}}{}}
    %         {\ifx\@institution\undefined\else\footerfont\@institution\hspace{10pt}\fi}
    %         {\ifx\@theday\undefined\else\footerfont\bfseries\@theday\hspace{10pt}\fi}
    %         {\ifx\@smalltitle\undefined\else\footerfont\@smalltitle\hspace{10pt}\fi}
    %         {\footerfont\textbf\thepage\textendash\pageref{LastPage}}
    %     }
    \fancyfoot[L]{\ifbool{printfoot}{\ifx\@footinfo\undefined\else\footerfont\@footinfo\fi}{}}
    \fancyfoot[C]{}
    \fancyhead[C]{}
    \fancyhead[R]{}
    \fancyhead[L]{}
}

% Header

\fancyhead[RE,LO]{
\ifbool{printhead}{
\copyright
    \ifx\@theyear\undefined\else\footerfont\@theyear\fi,~%
    \ifx\@journalname\undefined\else\footerfont\@journalname\fi,~%
    e-ISSN:~\ifx\@eissn\undefined\else\footerfont\@eissn\fi,~%
    \ifx\@theyear\undefined\else\footerfont\@theyear\fi,~%
    v.\ifx\@thevolume\undefined\else\footerfont\@thevolume\fi,~%
    \ifx\@articlenum\undefined\else\footerfont\@articlenum\fi%
}{}
}%\ifx\@title\undefined\else\footerfont\@title\fi}
\fancyhead[RO,LE]{\ifbool{printhead}{
\ifx\@leadauthor\undefined\else\footerfont\@leadauthor\fi
}{}}


% Footer
\fancyfoot[C]{\ifbool{printfoot}{\footerfont\bfseries\thepage}{}}

\renewcommand{\headrulewidth}{\ifbool{printhead}{0.4pt}{0pt}} % No header rule
\renewcommand{\footrulewidth}{0pt} % No footer rule

%----------------------------------------------------------
% RCTART START ~ LETTRINE
%----------------------------------------------------------

\newcommand{\dropcapfont}{\color{rctartcolor}\bfseries}%\fontsize{25pt}{28pt}\selectfont}
\newcommand{\rctartstart}[1]{\lettrine[lines=2,lraise=0,findent=2pt, nindent=0em]{{\dropcapfont{#1}}}{}}

%----------------------------------------------------------
% CORRESPONDING AUTHOR SECTION
%----------------------------------------------------------

% New commands
\newcommand{\rctartinfofont}{\color{black}\normalfont\sffamily\scriptsize\linespread{1}}%\fontsize{7.8}{9}\selectfont}
\newcommand{\received}[1]{\def\@received{#1}}
\newcommand{\revised}[1]{\def\@revised{#1}}
\newcommand{\accepted}[1]{\def\@accepted{#1}}
\newcommand{\published}[1]{\def\@published{#1}}
\newcommand{\corres}[1]{\def\@corres{#1}}
\newcommand{\email}[1]{\def\@email{#1}}
\newcommand{\doi}[1]{\def\@doi{#1}}
\newcommand{\license}[1]{\def\@license{#1}}

\newcommand{\eissn}[1]{\def\@eissn{#1}}
\newcommand{\articlenum}[1]{\def\@articlenum{#1}}

\newcommand{\rctartinfo}{
    \rctartinfofont
        \vskip5pt
            {\ifx\@corres\undefined\else\textcolor{rctartcolor}{\bfseries\ignorespaces\corresname}\@corres\fi} 
            {\ifx\@email\undefined\else\hspace{10pt}\textcolor{rctartcolor}{\bfseries\ignorespaces\emailname}\@email\fi\par}
            {\ifx\@doi\undefined\else\textcolor{rctartcolor}{\bfseries DOI: }\@doi\fi\par}
            {\ifx\@received\undefined\else\textcolor{rctartcolor}{\bfseries\ignorespaces\receivedname}\@received\hspace{10pt}\fi}
            {\ifx\@revised\undefined\else\textcolor{rctartcolor}{\bfseries\ignorespaces\revisedname}\@revised\hspace{10pt}\fi} 
            {\ifx\@accepted\undefined\else\textcolor{rctartcolor}{\bfseries\ignorespaces\acceptedname}\@accepted\hspace{10pt}\fi}
            {\ifx\@published\undefined\else\textcolor{rctartcolor}{\bfseries\ignorespaces\publishedname}\@published\fi\par}
                \vskip3pt
            {\@license\par}
         \vskip1pt
        \rule{\textwidth}{0.3pt}
}

%----------------------------------------------------------
% ABSTRACT STYLE
%----------------------------------------------------------

% Abstract text style new commands
\newcommand{\keywords}[1]{\def\@keywords{#1}}
\newcommand{\keywordsfont}{\normalfont\rmfamily\itshape\linespread{1}\small}%\fontsize{7.8}{9}\selectfont}
\newcommand{\keywordheadfont}{\normalfont\bfseries\rmfamily\small}                   %\fontsize{7.8}{9}\selectfont\bfseries}
\newcommand{\printkeywords}[2][\keywordname]{\vskip10pt\par\noindent\keywordheadfont #1:\space\hspace{0.1em}\keywordsfont{#2}}

% \newcommand{\palavraschave}[1]{\def\@palavraschave{#1}}
% \newcommand{\absfont}{\color{black}\normalfont\rmfamily\linespread{1}\normalsize}%\fontsize{8.5}{11}\selectfont}
% \newcommand{\absheadfont}{\color{rctartcolor}\normalfont\rmfamily\bfseries\scshape\large}    %\fontsize{9}{11}\selectfont}



% % Abstract definition
% \RequirePackage[style]{abstract}
% 
% \renewcommand{\abstractnamefont}{\normalfont\color{rctartcolor}\large\bfseries}
% \renewcommand{\abstracttextfont}{\normalfont\color{black}\normalsize}
% \renewcommand{\abstitlestyle}[1]{\centering\abstractnamefont\MakeUppercase{#1}}
% \setlength{\absleftindent}{0pt}
% \setlength{\absrightindent}{0pt}
% \setlength{\absparindent}{0pt}
% \setlength{\abstitleskip}{0pt}

% \def\xabstract{abstract}
% \long\def\abstract#1\end#2{
%     \def\two{#2}\ifx\two\xabstract
%     \long\gdef\theabstract{\ignorespaces#1}
%     \def\go{\end{abstract}}
% \else
%     #1\end{#2}
%     \gdef\theabstract{\vskip12pt 
%     \vskip12pt}
%     \let\go\relax\fi
% \go}

% Rctart class abstract style 
% \newcommand{\rctartabstract}{
%     \begin{tcolorbox}[colback=white, colframe=white, arc=0pt, boxrule=0pt, left=0pt, right=0pt, top=5pt, bottom=5pt]
%         \ifx\@keywords\undefined
%         {\centering\absheadfont\abstractname\par}\vskip0.5em{\absfont\theabstract}\vskip10pt
%         \else
%             {\centering\absheadfont\abstractname\par}\vskip0.5em{\absfont\theabstract}\vskip10pt
%             \keywordsfont\keywordname\hspace{0.1em}\keywordsfont\@keywords
%         \fi
%     \end{tcolorbox}
% }


%----------------------------------------------------------
% TITLE STYLE
%----------------------------------------------------------

% New commands
\newcommand{\journalname}[1]{\def\@journalname{#1}}
\newcommand{\dates}[1]{\def\@dates{#1}}
\newcommand{\datesfont}{\fontsize{7.5}{9}\selectfont}


\newcommand{\articleinfofont}{\footnotesize}
\newcommand{\articleinfo}{
    \articleinfofont
    %     \vskip5pt
    {\ifx\@theday\undefined\else\textcolor{rctartcolor}
        {\sffamily\bfseries\ignorespaces\@theday}\par\fi}        
    {\ifx\@received\undefined\else\textcolor{rctartcolor}
        {\sffamily\bfseries\ignorespaces\receivedname:\space}\@received\hspace{10pt}\fi}
    {\ifx\@revised\undefined\else\textcolor{rctartcolor}
        {\sffamily\bfseries\ignorespaces\revisedname:\space}\@revised\hspace{10pt}\fi}
    {\ifx\@accepted\undefined\else\textcolor{rctartcolor}
        {\sffamily\bfseries\ignorespaces\acceptedname:\space}\@accepted\hspace{10pt}\fi}
    {\ifx\@published\undefined\else\textcolor{rctartcolor}
        {\sffamily\bfseries\ignorespaces\publishedname:\space}\@published\fi\par}
    {\ifx\@eissn\undefined\else\textcolor{rctartcolor}
        {\sffamily\bfseries\ignorespaces e-ISSN:\space}\@eissn\hspace{10pt}\fi}
    {\ifx\@doi\undefined\else\textcolor{rctartcolor}
        {\sffamily\bfseries\ignorespaces DOI:\space}\@doi\hspace{10pt}\fi}
    {\ifx\@articlenum\undefined\else\textcolor{rctartcolor}
        {\sffamily\bfseries\ignorespaces Artigo:\space}\@articlenum\fi}
    \vskip3pt
    {\@license\par}
}

\newcommand{\aligntitleauthor}{\centering} %\RaggedRight
\newcommand{\alignsection}{\centering} %\RaggedRight
\newcommand{\abstractalign}{center title}
\newcommand{\rcttitlefont}{\color{rctartcolor}\rmfamily\bfseries\fontsize{14}{20}\selectfont}

\renewcommand{\@maketitle}{
    \bgroup\setlength{\parindent}{0pt}
    {
        \ifbool{tarja-info}{
        \ifx\@journalname\undefined\vskip-18pt
        \else
        \vspace*{-1.2cm}
        \begin{tcolorbox}[colupper=rcttexttarja, colback=rcttarja, arc=0pt, boxrule=0pt, left=5pt, right=5pt, top=4pt, bottom=3pt]
            \RaggedRight\sffamily\large\@journalname
        \end{tcolorbox}\fi
        \ifthenelse{\boolean{articleinfotop}}{
        {\ifx\articleinfo\undefined\vskip2pt\else
            \ifbool{article-info}{\RaggedRight\articleinfo}{}\fi}
%         \vskip-13pt
        \rule{\textwidth}{0.3pt}}{}}{}
        \par\vskip12pt
        {\aligntitleauthor\rcttitlefont\MakeUppercase{\@title}\par}
        \vskip12pt
        {\aligntitleauthor\@author\par}
        \vskip8pt
        {\ifx\@dates\undefined\vskip2pt\else\RaggedRight\datesfont\@dates\par\vskip0pt\fi}
        \ifthenelse{\boolean{articleinfotop}}{}{
        {\ifx\articleinfo\undefined\vskip2pt\else
            \ifbool{article-info}{\RaggedRight\articleinfo}{}\fi}
%         \vskip-13pt
        \rule{\textwidth}{0.3pt}
        \par\vskip6pt}
%         \ifbool{rctart-abstract}{\RaggedRight\rctartabstract}{}
%          \rule{\textwidth}{0.4pt}
    } 
    \egroup
    \vskip12pt
}

%----------------------------------------------------------
% AFFILIATION SETUP
%----------------------------------------------------------

\setlength{\affilsep}{9pt}
\renewcommand\Authfont{\normalfont\mdseries\sffamily}%\fontsize{9}{11}\selectfont}
\renewcommand\Affilfont{\it\small}%\fontsize{7.5}{9}\selectfont}
\renewcommand\AB@affilsep{\reset@font\protect\Affilfont}
\renewcommand\AB@affilsepx{\reset@font\protect\\\protect\Affilfont}
% \renewcommand\AB@affilsepx{, \protect\Affilfont}

% Authands language
\renewcommand\Authand{\ignorespaces\andname }
\renewcommand\Authands{\ignorespaces\andname }

%----------------------------------------------------------
% SECTION STYLE
%----------------------------------------------------------

\setcounter{secnumdepth}{5}

\newcommand{\partfont}{\color{rctartcolor}\large\bfseries}
\newcommand{\sectionfont}{\color{rctartcolor}\normalsize\bfseries}
\newcommand{\subsectionfont}{\bfseries}
\newcommand{\subsubsectionfont}{\small\bfseries\itshape}
\newcommand{\paragraphfont}{\small\bfseries}

\titleformat{\part}[block]
    {\partfont\alignsection}
    {\thepart.}
    {0.5em}
    {\MakeUppercase{#1}}
    []

\titleformat{\section}[block]
    {\sectionfont\alignsection}
    {\thesection.}
    {0.5em}
    {\MakeUppercase{#1}}
    []

% \titleformat{\section}
%     {\color{rctartcolor}\large\bfseries}
%     {\thesection.}
%     {0.5em}
%     {#1}
%     []
    
\titleformat{name=\section,numberless}[block]
    {\sectionfont\bfseries}
    {}
    {0em}
    {\tikz\draw[rctartcolor, fill=rctartcolor] (0,0) rectangle (6.6pt,6.6pt); \hspace{2.5pt} {#1}}
    []
  
\titleformat{\subsection}[block]
    {\subsectionfont}
    {\thesubsection.}
    {0.5em}
    {#1}     
    []
  
\titleformat{\subsubsection}[block] % You may change it to "runin"
    {\subsubsectionfont}
    {\thesubsubsection.}
    {0.5em}
    {#1}      % If using runin, change #1 to '#1. ' (space at the end)
    []    
  
\titleformat{\paragraph}[runin]
    {\paragraphfont}
    {}
    {0em}
    {#1} 
  
\titlespacing*{\section}{0pc}{2.5ex \@plus3pt \@minus2pt}{5pt}
\titlespacing*{\subsection}{0pc}{2.5ex \@plus3pt \@minus2pt}{2pt}
\titlespacing*{\subsubsection}{0pc}{2ex \@plus2.5pt \@minus1.5pt}{2pt}
\titlespacing*{\paragraph}{0pc}{1.5ex \@plus2pt \@minus1pt}{12pt}

%----------------------------------------------------------
% TABLE OF CONTENTS
%----------------------------------------------------------

\newlength{\tocsep} 
\setlength\tocsep{1.5pc} % Sets the indentation of the sections in the table of contents
\setcounter{tocdepth}{5} % Three levels in the table of contents section: sections, subsections and subsubsections

\usepackage{titletoc}
\contentsmargin{0cm}

%!TEX If using numberless sections we recomend to change the values 

\titlecontents{section}[\tocsep] % \setlength\tocsep{0c}
    {\addvspace{4pt}\sffamily\selectfont\bfseries}
    {\contentslabel[\thecontentslabel]{\tocsep}}
    {}
    {\hfill\thecontentspage}
    []

\titlecontents{subsection}[3pc] % 1pc
    {\addvspace{4pt}\small\sffamily\selectfont}
    {\contentslabel[\thecontentslabel]{\tocsep}}
    {}
    {\ \titlerule*[.5pc]{.}\ \thecontentspage}
    []

\titlecontents*{subsubsection}[3pc] % 1pc
    {\footnotesize\sffamily\selectfont}
    {}
    {}
    {}
    [\ \textbullet\ ]

%----------------------------------------------------------
% FIGURE-, TABLE-, LISTINGS- CAPTION STYLE
%----------------------------------------------------------

% General captions
\DeclareCaptionFont{rctartcolor}{\color{rctartcolor}}

\RequirePackage[
    labelfont={bf,sf},
    list=no,
    labelsep=period,
    singlelinecheck=off,
    %font=normalsize,%small,
    labelfont={normalsize,bf,rctartcolor},%
    textfont={normalsize},
    justification=centering,
    position=top
]{caption}

% Table caption
% \captionsetup[table]{position=top}
% \captionsetup[figure]{position=top}
% \newcommand{\tabletext}[1]{{\setlength{\leftskip}{9pt}\fontsize{7}{9}\vskip2pt\selectfont#1}}
\newcommand{\notetext}[1]{{\setlength{\leftskip}{9pt}\small\vskip2pt#1}}


% \captionsetup[lstlisting]{font=normalsize, labelfont={bf,sf}, belowskip=3pt, position=top, labelsep=period}
\renewcommand\lstlistingname{\captioncodename}
\renewcommand\lstlistlistingname{\captioncodename}

%----------------------------------------------------------
% LISTINGS STYLE
%----------------------------------------------------------

% Defined colors for listings environment
\definecolor{rctartcodeback}{RGB}{248, 248, 248}  
% \definecolor{rctartcodeback}{RGB}{255, 255, 255}     % Alternative back color
\definecolor{rctartcodecomment}{RGB}{1, 136, 0}
\definecolor{rctartcodekey}{RGB}{53, 53, 128}
\definecolor{rctartcodestring}{RGB}{122, 36, 47}
\definecolor{rctartgray}{RGB}{0.5,0.5,0.5}
\definecolor{rctartredmatlab}{RGB}{199, 78, 0}

% Rctart codes style
\lstdefinestyle{rctartcoding}{
    backgroundcolor=\color{rctartcodeback},   
    commentstyle=\color{rctartcodecomment},
    keywordstyle=\color{rctartcodekey},
    numberstyle=\tiny\color{rctartgray},
    stringstyle=\color{rctartcodestring},
    breakatwhitespace=false,        
    basicstyle=\small\ttfamily,
    breaklines=true,                 
    captionpos=b,                    
    keepspaces=true,                 
    numbers=left,				 	% if "none" change the values  
    numbersep=8pt,    				% 0pt              
    showspaces=false,                
    showstringspaces=false,
    showtabs=false,                  
    tabsize=2,
    aboveskip=12pt,
    belowskip=8pt,
    xleftmargin=10pt,				% 0pt
    rulecolor=\color{rctartgray},
    frame=single
}

% Matlab
\lstset{
    language=Matlab,
    rulecolor=\color{rctartgray},
    emph=[1]{for,end,break},
    emphstyle=[1]\color{rctartredmatlab},
}

\lstset{
    style=rctartcoding,
    captionpos=top,
    belowcaptionskip=10pt,
}

% veja https://stackoverflow.com/questions/1116266/listings-in-latex-with-utf-8-or-at-least-german-umlauts
\lstset{%
    inputencoding=utf8,
    extendedchars=true,
    literate=%
    {é}{{\'{e}}}1
    {è}{{\`{e}}}1
    {ê}{{\^{e}}}1
    {ë}{{\¨{e}}}1
    {É}{{\'{E}}}1
    {Ê}{{\^{E}}}1
    {û}{{\^{u}}}1
    {ù}{{\`{u}}}1
    {ú}{{\'{u}}}1
    {â}{{\^{a}}}1
    {à}{{\`{a}}}1
    {á}{{\'{a}}}1
    {ã}{{\~{a}}}1
    {Á}{{\'{A}}}1
    {Â}{{\^{A}}}1
    {Ã}{{\~{A}}}1
    {ç}{{\c{c}}}1
    {Ç}{{\c{C}}}1
    {õ}{{\~{o}}}1
    {ó}{{\'{o}}}1
    {ô}{{\^{o}}}1
    {Õ}{{\~{O}}}1
    {Ó}{{\'{O}}}1
    {Ô}{{\^{O}}}1
    {î}{{\^{i}}}1
    {Î}{{\^{I}}}1
    {í}{{\'{i}}}1
    {Í}{{\~{Í}}}1
}

%----------------------------------------------------------
% FOOTNOTE STYLE
%----------------------------------------------------------

\definecolor{black50}{gray}{0.5}
\renewcommand*{\footnotelayout}{\normalfont\fontsize{6}{8}}
\renewcommand{\footnoterule}{
    \kern -3pt
    {\color{black50} \hrule width 75pt height 0.25pt}
    \kern 2.5pt
}

%----------------------------------------------------------
% BIBLATEX
%----------------------------------------------------------

% \usepackage[
%     backend=biber,
%     style=ieee,
%     sorting=ynt
% ]{biblatex}
% 
% \addbibresource{rctart.bib}
\usepackage[alf]{abntex2cite}
\bibliographystyle{abntex2-alf}

%----------------------------------------------------------

%----------------------------------------------------------
% Typing environment, packages, commands
%----------------------------------------------------------

\newcommand{\nipkg}{\texttt}
\newcommand{\env}{\texttt}
\newcommand{\cls}{\texttt}
\newcommand{\fnt}{\texttt}
\newcommand{\opt}{\texttt}
\newcommand{\filn}{\texttt}

\endinput
