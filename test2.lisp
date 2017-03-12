;; at highest level it will look like this:

(window
 (grid :rows 2
  (button :grid-row 0 :click (lambda () (print "I was clicked")))
  (text-block :grid-row 1 :content "Please click the button")))


;; I dont yet have support for grids in the base library
;; stackpanels I do however.
;; In any case, user control types can be defined in lisp code or in c
;; Since I want to be able to do this persisted I will have to do some tricks with lambda functions. I am going to take this on a per-type bases, but at least things need an ID.

(defvar win
  (window :id "win1" :width 500 :height 500
	  (button)))

(show win)
