<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [
<!ENTITY walsh-style PUBLIC "-//Norman Walsh//DOCUMENT DocBook HTML Stylesheet//EN" CDATA DSSSL>
<!ENTITY cygnus-style SYSTEM "/usr/lib/sgml/stylesheet/dsssl/docbook/cygnus/cygnus-both.dsl" CDATA DSSSL>
]>

<style-sheet>
<style-specification id="html" use="docbook">
<style-specification-body>

(define %use-id-as-filename% #t)
(define %html-ext% ".html")
(define %html-header-tags% '())

;;(define %stylesheet% "../../winehq.css")
;;(define %stylesheet-type% "text/css")

(define %shade-verbatim% #t)
(define %section-autolabel% #t)

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

;; Customize systemitem element to have different formatting, according
;; to which class attribute it contains.
(element systemitem
  (let ((class (attribute-string (normalize "class"))))
    (cond
     ((equal? class (normalize "systemname")) ($italic-mono-seq$))
     ((equal? class (normalize "constant")) ($mono-seq$))
     (else ($charseq$)))))

;; Okay, this is a little tricky.  By default, it appears that setinfo is
;; completely turned off (with empty-sosofo).  The setinfo title is extracted
;; through some other means, so we can ignore it when we process the setinfo
;; below.

;; Process setinfo element
(element setinfo (process-children))
;; Ignore title element -- otherwise it'll appear alongside the releaseinfo
;; element.  If we add any other elements to setinfo, we'll have to blank them
;; out here, also.
(element (setinfo title)
  (empty-sosofo))
;; Enclose releaseinfo element in italics
(element (setinfo releaseinfo)
;  (make element gi: "i"
;  (process-children)))
  (process-children))

</style-specification-body>
</style-specification>

<external-specification id="docbook" document="walsh-style">

</style-sheet>
