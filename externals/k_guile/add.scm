
(pd-inlets 2)
(pd-outlets 1)

(let ((inlet1 0))
  (pd-inlet 1 'float
	    (lambda (x)
	      (set! inlet1 x)))
  (pd-inlet 0 'float
	    (lambda (x)
	      (pd-outlet 0 (+ inlet1 x)))))




