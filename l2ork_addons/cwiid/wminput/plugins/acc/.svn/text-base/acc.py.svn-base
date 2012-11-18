import wmplugin
import cwiid
import math

acc_zero = None
acc_one = None
acc = [0,0,0]

NEW_AMOUNT = 0.1
OLD_AMOUNT = 1 - NEW_AMOUNT

Roll_Scale = 1
Pitch_Scale = 1
X_Scale = 1
Y_Scale = 1

def wmplugin_info():
	return [], \
	  [("Roll", wmplugin.ABS, 3141, -3141, 0, 0), \
	   ("Pitch", wmplugin.ABS, 1570, -1570, 0, 0), \
	   ("X", wmplugin.ABS | wmplugin.REL, 16, -16, 0, 0), \
	   ("Y", wmplugin.ABS | wmplugin.REL, 16, -16, 0, 0)], \
	  [("Roll_Scale", wmplugin.PARAM_FLOAT, Roll_Scale), \
	   ("Pitch_Scale", wmplugin.PARAM_FLOAT, Pitch_Scale), \
	   ("X_Scale", wmplugin.PARAM_FLOAT, X_Scale), \
	   ("Y_Scale", wmplugin.PARAM_FLOAT, Y_Scale)]

def wmplugin_init(id, wiimote):
	global acc_zero, acc_one

	wmplugin.set_rpt_mode(id, cwiid.RPT_ACC)
	acc_zero, acc_one = wiimote.get_acc_cal(cwiid.EXT_NONE)
	return

def wmplugin_exec(mesg):
	global acc_zero, acc_one, acc
	axes = [None, None, None, None]

	for m in mesg:
		if m[0] == cwiid.MESG_ACC:
			acc = [NEW_AMOUNT*(new-zero)/(one-zero) + OLD_AMOUNT*old
			       for old,new,zero,one in zip(acc,m[1],acc_zero,acc_one)]
			a = math.sqrt(sum(map(lambda x: x**2, acc)))

			roll = math.atan(acc[cwiid.X]/acc[cwiid.Z])
			if acc[cwiid.Z] <= 0:
				if acc[cwiid.X] > 0: roll += math.pi
				else: roll -= math.pi

			pitch = math.atan(acc[cwiid.Y]/acc[cwiid.Z]*math.cos(roll))

			axes[0] = int(roll  * 1000 * Roll_Scale)
			axes[1] = int(pitch * 1000 * Pitch_Scale)

			if (a > 0.85) and (a < 1.15):
				if (math.fabs(roll)*(180/math.pi) > 10) and \
				   (math.fabs(pitch)*(180/math.pi) < 80):
					axes[2] = int(roll * 5 * X_Scale)

				if (math.fabs(pitch)*(180/math.pi) > 10):
					axes[3] = int(pitch * 10 * Y_Scale)

	return [], axes

