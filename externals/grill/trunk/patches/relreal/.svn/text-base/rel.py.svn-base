import ode
import pyext
import math
import random

try:
    import psyco
    psyco.full()
    print "Using Psyco JIT compilation"
except:
    pass

print "Relative Realitäten!"

def spheremass(d,r):
    return 4*r*r*r*math.pi/3*d

def sqr(x):
    return x*x

def vminus(p1,p2):
    return (p1[0]-p2[0],p1[1]-p2[1],p1[2]-p2[2])

def vnorm(p):
    return math.sqrt(sqr(p[0])+sqr(p[1])+sqr(p[2]))

def vdist(p1,p2):
    return vnorm(vminus(p1,p2))

def vmul(p,f):
    return (p[0]*f,p[1]*f,p[2]*f)

def setrest(body,pos):
    body.setPosition( pos )
    body.setLinearVel( (0,0,0) )
    body.setAngularVel( (0,0,0) )


s_pendulum = pyext.Symbol("pendulum")
s_anchor = pyext.Symbol("anchor")
s_sat = pyext.Symbol("sat")
s_pos = pyext.Symbol("pos")


class Object:
    body = None
    geom = None
    m = 0  # mass

    def __init__(self,world,i,grav):
        self.body = ode.Body(world)
        self.index = i
        self.body.setGravityMode(grav)

    def getPosition(self):
        return self.body.getPosition()

    def getLinearVel(self):
        return self.body.getLinearVel()

    def getAngularVel(self):
        return self.body.getAngularVel()

    def setPosition(self,pos):
        self.body.setPosition(pos)

    def resetPosition(self,pos):
        self.setPosition(pos)
        self.body.setLinearVel((0,0,0))
        self.body.setAngularVel((0,0,0))

    def grav(self,G,second):
        vec = vminus(second.getPosition(),self.getPosition())
        d = vnorm(vec)
        f = G*self.m*second.m/(d*d)
        self.body.addForce(vmul(vec,f/d))

    def brake(self,F):
        self.body.addForce(vmul(self.body.getLinearVel(),-F))


class Sphere(Object):

    def __init__(self,world,space,index,density,radius,grav):
        Object.__init__(self,world,index,grav)
        
        m = ode.Mass()
        m.setSphere(density,radius)
        self.body.setMass(m)

        # Create a geom for collision detection
        self.geom = ode.GeomSphere(space, radius=radius)
        self.geom.setBody(self.body)

        self.m = spheremass(density,radius)

    def setDensity(self,density):
        radius = self.geom.getRadius()
        m = self.body.getMass()
        m.setSphere(density,radius)
        
        self.m = spheremass(density,radius)



class rel(pyext._class):

    G = 0.01 # gravitation constant
    F = 0   #  fluidum breaking 
    
    g = 9.81
    l = 10 # pendulum length
    y = 1 # pendulum elongation

    d = 10000 #density
    r = 1 # pendulum radius
    ds = 1000 #density
    rs = 0.1 # statellite radius
    
    inter = False  # inter-satellite gravitation

    origin = (0,0,0)    
    
    sjoint = []
    bodies = None

    def nearcb(self, args, geom1, geom2):
        """Create contact joints between colliding geoms"""

        body1, body2 = geom1.getBody(), geom2.getBody()
        
        pos = ()
        
        if body1 is None:
            body1 = ode.environment
        else:
            pos = body1.getPosition()
            
        if body2 is None:
            body2 = ode.environment
        else:
            pos = body2.getPosition()

        if ode.areConnected(body1, body2):
            return

        contacts = ode.collide(geom1, geom2)

        for c in contacts:
#            c.setBounce(0.2)
#            c.setMu(10000)
            c.setBounce(1)
            c.setMu(0)
            j = ode.ContactJoint(self.world, self.contactgroup, c)
            j.attach(body1, body2)

            # report collision
            self._outlet(3,(self.geoms[geom1],self.geoms[geom2])+pos)

    def __init__(self,num):       
        # Create a world object
        self.world = ode.World()
        self.world.setGravity( (0,-self.g,0) )
        self.world.setERP(0.8)
        self.world.setCFM(1.e-5)

        self.space = ode.Space()
        self.contactgroup = ode.JointGroup()

        # create objects
        self.anchor = Sphere(self.world,self.space,-2,1.e30,0.001,False)
        self.pendulum = Sphere(self.world,self.space,-1,self.d,self.r,True)

        self.joint = ode.BallJoint(self.world)
        self.joint.attach(self.anchor.body,self.pendulum.body)

        self.sat = [Sphere(self.world,self.space,s,self.ds,self.rs,False) for s in range(num)]
        
        self.walls = (
            ode.GeomPlane(self.space, ( 1, 0, 0), -(self.l+self.r)*1.1),
            ode.GeomPlane(self.space, ( 0, 1, 0), -(self.l+self.r)*1.1),
            ode.GeomPlane(self.space, ( 0, 0, 1), -(self.l+self.r)*1.1),
            ode.GeomPlane(self.space, (-1, 0, 0), -(self.l+self.r)*1.1),
            ode.GeomPlane(self.space, ( 0,-1, 0), -self.l*0.1),
            ode.GeomPlane(self.space, ( 0, 0,-1), -(self.l+self.r)*1.1)
        )

        # remember bodies
        self.geoms = {}
        self.geoms[self.anchor.geom] = -2
        self.geoms[self.pendulum.geom] = -1
        for s in range(num):
            self.geoms[self.sat[s].geom] = s
        
        for s in range(len(self.walls)):
            self.geoms[self.walls[s]] = 1000+s
        
        self.reset_1()

    def reset_1(self):
        '''reset object position and motion'''
        self.time = 0.0
        
        self.anchor.resetPosition(self.origin)
        self.pendulum.resetPosition((math.sqrt(self.l*self.l-self.y*self.y),-self.y,0))

        n = len(self.sat)
        for i in range(n):
            x = (i-n/2.)*self.rs*2.5
            y = -random.random()*self.rs-self.l
            z = (random.random()-0.5)*self.rs
            self.sat[i].resetPosition((x,y,z))

        self.joint.setAnchor(self.anchor.getPosition())
        

    def density_1(self,i,d):
        '''set density'''
        if i < 0:
            self.pendulum.setDensity(d)
        else:
            self.sat[i].setDensity(d)

    def pos_1(self,i,*p):
        '''set position'''
        if i < 0:
            self.pendulum.setPosition(p)
        else:
            self.sat[i].setPosition(p)

    def step_1(self,dt):
        '''make a time step'''
        
        if dt <= 0:
            print "Stepsize must be > 0"
            return

        # gravitation satellite-pendulum
        # viscosity
        for s in self.sat:
            s.grav(self.G,self.pendulum)
            s.brake(self.F)

        # inter-satellite gravitation
        if self.inter:
            for s1 in range(len(self.sat)):
                for s2 in range(s1+1,len(self.sat)):
                    self.sat[s1].grav(self.G,self.sat[s2])
                    self.sat[s2].grav(self.G,self.sat[s1])

        # Do the simulation...
        self.space.collide((),self.nearcb)
        
        self.world.step(dt)

        self.time += dt
        self.contactgroup.empty()

        # correct anchor position
        self.anchor.setPosition(self.origin)

        # output pendulum position
        self._outlet(1,self.pendulum.getPosition())

        # output satellite data
        for i,sat in enumerate(self.sat):
            self._outlet(2,(i,)+sat.getPosition()+sat.getLinearVel()+sat.getAngularVel())
