<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [
<!ENTITY walsh-style PUBLIC "-//Norman Walsh//DOCUMENT DocBook HTML Stylesheet//EN" CDATA DSSSL>
<!ENTITY cygnus-style SYSTEM "/usr/lib/sgml/stylesheet/dsssl/docbook/cygnus/cygnus-both.dsl" CDATA DSSSL>
]>

<style-sheet>
<style-specification id="html" use="docbook">
<style-specification-body>

(define %use-id-as-filename% #t)
(define %html-ext% ".shtml")
(define %html-header-tags% '())

(define %stylesheet% "../../winehq.css")
(define %stylesheet-type% "text/css")

(define %shade-verbatim% #t)
(define %section-autolabel% #t)

;; Define new HTML headers
(define ($html-body-start$)
  (make sequence
    (make formatting-instruction data: "&#60!--")
    (literal "#include file=\"header.html\" ")
    (make formatting-instruction data: "-->")))
(define ($html-body-end$)
  (make sequence
    (make formatting-instruction data: "&#60!--")
    (literal "#include file=\"footer.html\" ")
    (make formatting-instruction data: "-->")))

;; Customize the body tag attributes
;;(define %body-attr% 
;;  (list
 ;;  (list "BGCOLOR" "#555555")
;;   (list "TEXT" "#000000")
;;   (list "LINK" "#0000FF")
;;   (list "VLINK" "#840084")
;;   (list "ALINK" "#0000FF")))

</style-specification-body>
</style-specification>

<external-specification id="docbook" document="walsh-style">

</style-sheet>
