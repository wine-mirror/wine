<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [
<!ENTITY walsh-style PUBLIC "-//Norman Walsh//DOCUMENT DocBook HTML Stylesheet//EN" CDATA DSSSL>
<!ENTITY cygnus-style SYSTEM "/usr/lib/sgml/stylesheet/dsssl/docbook/cygnus/cygnus-both.dsl" CDATA DSSSL>
]>

<style-sheet>
<style-specification id="html" use="docbook">
<style-specification-body>

; Use the section id as the filename rather than
; cryptic filenames like x1547.html
(define %use-id-as-filename% #t)

; Repeat the section number in each section to make it easier
; when browsing the doc
(define %section-autolabel% #t)

;;(define %stylesheet% "../../winehq.css")
;;(define %stylesheet-type% "text/css")

; All that remains is to hard-code various aspects of the look and feel
; (colors, italics, etc.)

(define %shade-verbatim% #t)

;; Customize the body tag color attributes
(define %body-attr% 
  (list
   (list "BGCOLOR" "#FFFFFF")
   (list "TEXT" "#000000")
   (list "LINK" "#a50d0d")
   (list "VLINK" "#505050")
   (list "ALINK" "#a50d0d")))

;; Change the background color of programlisting and screen, etc.
(define ($shade-verbatim-attr$)
  (list
   (list "BORDER" "0")
   ;(list "BGCOLOR" "#E0E0E0")  ; light grey
   (list "BGCOLOR" "#E0D0D0")  ; light grayish red
   ;(list "BGCOLOR" "#bc8686")  ; dark rose
   ;(list "BGCOLOR" "#FFD39B")  ; burlywood1 (tan)
   ;(list "BGCOLOR" "#FFE7BA")  ; wheat1 (light tan)
   (list "WIDTH" ($table-width$))))

</style-specification-body>
</style-specification>

<external-specification id="docbook" document="walsh-style">

</style-sheet>
