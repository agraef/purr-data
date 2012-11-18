
(define num-inouts 400)

(pd-inlets num-inouts)
(pd-outlets num-inouts)


(pd-for 0 < num-inouts 1
	(lambda (i)
	  (pd-inlet i 'float
		    (lambda (x)
		      (pd-display "Got " x " to inlet " i)
		      (pd-for 0 < num-inouts 1
			      (lambda (i2)
				(pd-outlet i2 (+ i2 x))))))))


(pd-inlet 284 'testing
	  (lambda ()
	    (pd-display "This is a function for handling 'testing sent to inlet 284.")))




