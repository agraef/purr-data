

;; This file is evaluated (not (load)-ed) right before the file defined in the k_guile object in pd is evaluated or the
;; reload message has been sent. (see k_guile.c/k_guile_new)
;; Kjetil S. Matheussen, 2004.
;;
;;/* This program is free software; you can redistribute it and/or                */
;;/* modify it under the terms of the GNU General Public License                  */
;;/* as published by the Free Software Foundation; either version 2               */
;;/* of the License, or (at your option) any later version.                       */
;;/*                                                                              */
;;/* This program is distributed in the hope that it will be useful,              */
;;/* but WITHOUT ANY WARRANTY; without even the implied warranty of               */
;;/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
;;/* GNU General Public License for more details.                                 */
;;/*                                                                              */
;;/* You should have received a copy of the GNU General Public License            */
;;/* along with this program; if not, write to the Free Software                  */
;;/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.  */
;;/*                                                                              */




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Instance data
;;
;;
(define pd-num-inlets 1)
(define pd-num-outlets 0)



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Argument checking
;;
;;
(define (pd-legaloutlet outlet-num)
  (if (and (< outlet-num pd-num-outlets) (>= outlet-num 0))
      #t
      (begin
	(pd-display "outlet-num out of range")
	#f)))

(define (pd-legalinlet inlet-num)
  (if (and (< inlet-num pd-num-inlets) (>= inlet-num 0))
      #t
      (begin
	(pd-display "inlet-num out of range")
	#f)))



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Inlets
;;
;;
(define pd-inlet-vector (make-vector 1 '()))
(define pd-inlet-anyvector (make-vector 1 '()))

;; This function is called from the C side when the object receives something on an inlet.
(define (pd-inlet-func inlet-num symbol args)
  (let ((inlet-func (assq symbol 
			  (vector-ref pd-inlet-vector
				      inlet-num))))
    (if (not inlet-func)
	(begin
	  (set! inlet-func (assq 'any
				 (vector-ref pd-inlet-vector inlet-num)))
	  (set! args (cons symbol args))))
    (if inlet-func
	(apply (cadr inlet-func) args)
	(pd-display "No function defined for handling \'" symbol " to inlet " inlet-num))))

(define (pd-inlet inlet-num symbol func)
  (if (not (procedure? func))
      (pd-display "Wrong argument to pd-inlet: " func " is not a procedure")
      (if (and (pd-check-number inlet-num "pd-inlet")
	       (pd-legalinlet inlet-num))
	  (let ((inlet-funcs (vector-ref (if (eq? symbol 'any)
					     pd-inlet-anyvector
					     pd-inlet-vector)
					 inlet-num)))
	    (vector-set! pd-inlet-vector 
			 inlet-num
			 (cons (list symbol func)
			       inlet-funcs))))))

(define (pd-inlets new-num-inlets)
  (let ((num-inlets (if (pd-c-inited? pd-instance)
			(pd-c-get-num-inlets pd-instance)
			new-num-inlets)))
    (if (pd-check-number num-inlets "pd-inlets")
	(if (<= num-inlets 0)
	    (pd-display "num-inlets must be greater than 0, not " num-inlets)
	    (begin
	      (set! pd-num-inlets num-inlets)
	      (set! pd-inlet-vector (make-vector num-inlets '()))
	      (pd-c-inlets pd-instance num-inlets))))))




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Outlets
;;
;;
(define (pd-outlets new-num-outlets)
  (let ((num-outlets (if (pd-c-inited? pd-instance)
			 (pd-c-get-num-outlets pd-instance)
			 new-num-outlets)))
    (if (pd-check-number num-outlets "pd-outlets")
	(if (<= num-outlets 0)
	    (pd-display "num-outlets must be greater than 0, not " num-outlets)
	    (begin
	      (set! pd-num-outlets num-outlets)
	      (pd-c-outlets pd-instance num-outlets))))))

(define (pd-outlet outlet-num firstarg . args)
  (if (pd-legaloutlet outlet-num)
      (cond ((> (length args) 0) (pd-c-outlet-list pd-instance outlet-num issymbol (cons firstarg args)))
	    ((list? firstarg) (pd-c-outlet-list pd-instance outlet-num firstarg))
	    ((number? firstarg) (pd-c-outlet-number pd-instance outlet-num firstarg))
	    ((string? firstarg) (pd-c-outlet-string pd-instance outlet-num firstarg))
	    ((eq? 'bang firstarg) (pd-c-outlet-bang pd-instance outlet-num))
	    ((symbol? firstarg) (pd-c-outlet-symbol pd-instance outlet-num firstarg))
	    (else
	     (pd-display "Unknown argument to pd-outlet-or-send:" firstarg)))))




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Bindings
;;
;;
;; We must have our own local bind/unbind functions to be able to clean up automaticly.
(define pd-local-bindings '())

(define (pd-bind symbol func)
  (set! pd-local-bindings (pd-bind-do symbol func pd-local-bindings)))

(define (pd-unbind symbol)
  (set! pd-local-bindings (pd-unbind-do symbol pd-local-bindings)))

(define (pd-unbind-all)
  (if (not (null? pd-local-bindings))
      (begin
	(pd-unbind (car (car pd-local-bindings)))
	(pd-unbind-all))))


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Cleanup
;;
;;
(define pd-destroy-func #f)
(define (pd-set-destroy-func thunk)
  (if (not (procedure? thunk))
      (pd-display "Wrong argument to pd-set-destroy-func: " thunk " is not a procedure.")
      (set! pd-destroy-func thunk)))

;; This func is called from the C-side.
(define (pd-cleanup-func)
  (if pd-destroy-func
      (begin
	(pd-destroy-func)
	(set! pd-destroy-func #f)))
  (pd-unbind-all))

