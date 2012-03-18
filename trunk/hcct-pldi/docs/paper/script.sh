#!/bin/bash

#prepare the DVI file:
#latex book.tex
#bibtex book.tex
#latex book.tex

#render the high resolution PDF from the DVI file:
export bookDVI=pldi149-delia.dvi
export bookPDF=pldi149-delia.pdf

#MANDRAKE 9.1:
#export DVIPS=dvips -j0 -Ppdf -Pdownload35 -G0 -t letter -D 1200 -Z -mode ljfzzz
export DVIPS="dvips -j0 -Pdownload35 -G0 -t letter -D 1200 -Z -mode ljfzzz"

#REDHAT 9 (ar-std-urw-kb.map,ar-ext-urw-kb.map : assume K Berry names - Kluwer however uses 'Dvipsone' names):
# Note: there is a bug in the font mapping on Redhat9 (mtsupp-std-urw-urw.map contains Nimbus...):
#       however, it works fine on Mandrake.
#DVIPS=dvips -j0 -Ppdf -u ps2pk.map -G0 -t letter -D 1200 -Z -mode ljfzzz

#SuSe:
# ... seems to be similar to Redhat9 ...

#Notes:
# -j0 -G0: tells dvips to embed the fonts 100%
# -Pdownload35 or -u ps2pk.map: configuration of PostScript Type 1 fonts
# - Dvipsone Names: they have to be activated in the style file of the publisher

${DVIPS} ${bookDVI} -o - | gs -q -dNOPAUSE -dBATCH \
	-sDEVICE=pdfwrite -dPDFSETTINGS=/prepress\
	-dCompatibilityLevel=1.3 \
	-dCompressPages=true -dUseFlateCompression=false \
	-dSubsetFonts=true -dEmbedAllFonts=true \
	-dProcessColorModel=/DeviceGray \
	-dDetectBlends=true -dOptimize=true \
	-dDownsampleColorImages=true -dColorImageResolution=1200 \
	-dColorImageDownsampleType=/Average -dColorImageFilter=/FlateEncode \
	-dAutoFilterColorImages=false -dAntiAliasColorImages=false \
	-dColorImageDownsampleThreshold=1.50000 \
	-dDownsampleGrayImages=true -dGrayImageResolution=1200 \
	-dGrayImageDownsampleType=/Average -dGrayImageFilter=/FlateEncode \
	-dAutoFilterGrayImages=false -dAntiAliasGrayImages=false \
	-dGrayImageDownsampleThreshold=1.50000 \
	-dDownsampleMonoImages=true -dMonoImageResolution=1200 \
	-dMonoImageDownsampleType=/Average -dMonoImageFilter=/FlateEncode \
	-dAutoFilterMonoImages=false -dAntiAliasMonoImages=false \
	-dMonoImageDownsampleThreshold=1.50000 \
	-sOutputFile=${bookPDF} \
	-c save pop -

#now you should find the PDF in the local directory.