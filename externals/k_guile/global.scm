

;; These functions are global functions available for all guile scripts loaded into PD.
;; Kjetil S. Matheussen, 2004.

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



(debug-enable 'debug)
(debug-enable 'trace)
(debug-enable 'backtrace)

(use-modules (ice-9 stack-catch))



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Misc. functions
;;
;;
(define (pd-load-if-exists filename)
  (if (access? filename F_OK)
      (load filename)))

(define (pd-display . args)
  (if (not (null? args))
      (begin
	(display (car args))
	(apply pd-display (cdr args)))
      (newline)))

(define (pd-filter proc list)
  (if (null? list)
      '()
      (if (proc (car list))
	  (cons (car list) (pd-filter proc (cdr list)))
	  (pd-filter proc (cdr list)))))

(define (pd-for init pred least add proc)
  (if (pred init least)
      (begin
	(proc init)
	(pd-for (+ add init) pred least add proc))))



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Argument checking
;;
;;
(define (pd-check-number number message)
  (if (number? number)
      #t
      (begin
	(pd-display message ": " number " is not a number")
	#f)))



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Bindings
;;
;;
(define pd-global-bindings '())

(define (pd-bind-do symbol func bindings)
  (if (or (not (symbol? symbol))
	  (not (procedure? func)))
      (begin
	(pd-display "Wrong arguments for pd-bind")
	bindings)
      (cons (list symbol 
		  func
		  (pd-c-bind symbol func))
	    bindings)))

(define (pd-unbind-do symbol bindings)
  (if (not (symbol? symbol))
      (begin
	(pd-display "Wrong arguments for pd-unbind")
	bindings)
      (let ((binding (assq symbol bindings)))
	(pd-c-unbind (caddr binding) symbol)
	(pd-filter (lambda (x) (not (eq? symbol (car x))))
		   bindings))))

(define (pd-bind symbol func)
  (set! pd-global-bindings (pd-bind-do symbol func pd-global-bindings)))

(define (pd-unbind symbol)
  (set! pd-global-bindings (pd-unbind-do symbol pd-global-bindings)))




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Sending
;;
;;
(define (pd-send symbol firstarg . args)
  (if (or (symbol? symbol)
	  (number? symbol))
      (cond ((> (length args) 0) (pd-c-send-list symbol (cons firstarg args)))
	    ((list? firstarg) (pd-c-send-list symbol firstarg))
	    ((number? firstarg) (pd-c-send-number symbol firstarg))
	    ((string? firstarg) (pd-c-send-string symbol firstarg))
	    ((eq? 'bang firstarg) (pd-c-send-bang symbol))
	    ((symbol? firstarg) (pd-c-send-symbol symbol firstarg))
	    (else
	     (pd-display "Unknown argument to pd-outlet-or-send:" firstarg)))))

(define (pd-get-symbol sym)
  (if (not (symbol? sym))
      (pd-display sym " is not a scheme symbol")
      (pd-c-get-symbol sym)))




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Backtrace (does not work properly)
;;
;;

(define (pd-backtrace-eval string)
  (eval-string string))

(define (pd-display-errorfunc key . args)
  (let ((dasstack (make-stack #t)))
    (display-backtrace dasstack (current-output-port) #f #f)
					;(display (stack-ref (make-stack #t) 1))
					;(display (stack-length (make-stack #t)))
    (display key)(newline)
    (display args)
    (newline))
  0)

(define (pd-backtrace-run thunk)
  (stack-catch #t
	       thunk
	       pd-display-errorfunc))

(define (pd-backtrace-runx func arg1) 
  (stack-catch #t
	       (lambda x
		 (apply func x))
	       pd-display-errorfunc))

(define (pd-backtrace-run1 func arg1)
  (stack-catch #t
	       (lambda ()
		 (func arg1))
	       pd-display-errorfunc))

(define (pd-backtrace-run2 func arg1 arg2)
  (stack-catch #t
	       (lambda ()
		 (func arg1 arg2))
	       pd-display-errorfunc))

(define (pd-backtrace-run3 func arg1 arg2 arg3)
  (stack-catch #t
	       (lambda ()
		 (func arg1 arg2 arg3))
	       pd-display-errorfunc))

(define (pd-backtrace-run4 func arg1 arg2 arg3 arg4)
  (stack-catch #t
	       (lambda ()
		 (func arg1 arg2 arg3 arg4))
	       pd-display-errorfunc))

(pd-backtrace-run1 pd-load-if-exists "/etc/.k_guile.scm")
(pd-backtrace-run1 pd-load-if-exists (string-append (getenv "HOME") "/.k_guile.scm"))

