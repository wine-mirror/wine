<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [ 
<!ENTITY print-ss PUBLIC "-//Norman Walsh//DOCUMENT DocBook Print Stylesheet//EN" CDATA DSSSL>
]>

<style-sheet>

<style-specification id="print" use="print-stylesheet">
<style-specification-body>

;; I was hoping that this would take out the many blank pages in the
;; PDF file, but it doesn't, it just slides the page numbers over.  
(define %two-side% #f)

(define %generate-book-titlepage% #t)

;;Titlepage Not Separate
(define (chunk-skip-first-element-list)
  (list (normalize "sect1")
	(normalize "section")))

;;Titlepage Separate?
;(define (chunk-skip-first-element-list) 
;  '())

(define (list-element-list)
  ;; fixes bug in Table of Contents generation
  '())

(define (toc-depth nd)
  2)

;; This seems to have no affect
(define %generate-book-titlepage-on-separate-page% #f)

(define %body-start-indent%
  ;; Default indent of body text
  2pi)

(define %para-indent-firstpara%
  ;; First line start-indent for the first paragraph
  0pt)

(define %para-indent%
  ;; First line start-indent for paragraphs (other than the first)
  0pt)

(define %block-start-indent%
  ;; Extra start-indent for block-elements
  2pt)

;;Define distance between paragraphs
(define %para-sep% 
 (/ %bf-size% 2.0))

;;Define distance between block elements (figures, tables, etc.).
(define %block-sep% 
 (* %para-sep% 1.0))
;; (* %para-sep% 2.0))

(define %hyphenation%
  ;; Allow automatic hyphenation?
  #t)

(define %left-margin% 5pi)
(define %right-margin% 5pi)
(define %top-margin% 5pi)
(define %bottom-margin% 5pi)

(define %footer-margin% 2pi)
(define %header-margin% 2pi)

(define %line-spacing-factor% 1.3)
  ;; Factor used to calculate leading
  ;; The leading is calculated by multiplying the current font size by the 
  ;; '%line-spacing-factor%'. For example, if the font size is 10pt and
  ;; the '%line-spacing-factor%' is 1.1, then the text will be
  ;; printed "10-on-11".

(define %head-before-factor% 
  ;; Factor used to calculate space above a title
  ;; The space before a title is calculated by multiplying the font size
  ;; used in the title by the '%head-before-factor%'.
;;  0.75)
  0.5)

(define %head-after-factor% 
  ;; Factor used to calculate space below a title
  ;; The space after a title is calculated by multiplying the font size used
  ;; in the title by the '%head-after-factor%'.
  0.5)

(define %input-whitespace-treatment% 'collapse)

(define ($generate-book-lot-list$)
  ;; Which Lists of Titles should be produced for Books?
  (list ))

(define tex-backend 
  ;; Are we using the TeX backend?
  ;; This parameter exists so that '-V tex-backend' can be used on the
  ;; command line to explicitly select the TeX backend.
  #t)

</style-specification-body>
</style-specification>

<external-specification id="print-stylesheet" document="print-ss">

</style-sheet>


