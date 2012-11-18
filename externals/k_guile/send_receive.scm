


;; Send out what comes in.
(pd-bind 'in
	 (lambda (arg)
	   (pd-send 'out arg)))


#!
;; This one does the same and is faster, but requires some more typing:
(let ((s-out (pd-get-symbol 'out)))
  (pd-bind 'in
	   (lambda (arg)
	     (pd-send s-out arg))))
!#


#!
;; And the following example will (most probably) lead to a segmentation fault:
;; This is also the only way I can think of right now that will make pd segfault using the pd- interface.
(pd-send 5 arg)
!#

