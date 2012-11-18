;;; pd-mode.el --- major mode for editing Pd configuration files

;; Author: Hans-Christoph Steiner <hans@at.or.at>
;; Keywords:    languages, faces
;; Last edit: 
;; Version: 1.0.1 

;; This file is an add-on for XEmacs or GNU Emacs (not tested with the latter).
;;
;; It is free software; you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2, or (at your option)
;; any later version.x
;;
;; It is distributed in the hope that it will be useful, but
;; WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;; General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with your copy of Emacs; see the file COPYING.  If not, write
;; to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
;; Boston, MA 02111-1307, USA.

;;; Commentary:
;;
;; There isn't really much to say.  The list of keywords was derived from
;; the Pd there may be some errors or omissions.
;;
;; There are currently no local keybindings defined, but the hooks are
;; there in the event that anyone gets around to adding any.
;;
;; To enable automatic selection of this mode when appropriate files are
;; visited, add the following to your favourite site or personal Emacs
;; configuration file:
;;
;;   (autoload 'pd-mode "pd-mode" "autoloaded" t)
;;   (add-to-list 'auto-mode-alist '("\\.pat$" . pd-mode))
;;   (add-to-list 'auto-mode-alist '("\\.pd$"  . pd-mode))
;;
 
;;; Code:

;; Requires
(require 'generic-x)

(define-generic-mode 'pd-mode
  nil
  '("osc~" "random" "route" "trigger")
  '(("#X \\([^ ]+\\) " . 'font-lock-function-name-face)
    (";" . 'font-lock-warning-face))
  '(".pd\\'" ".pat\\'")
  nil
  "Major mode for editing Pd files")

;;; pd-mode.el ends here
