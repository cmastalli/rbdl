#include <UnitTest++.h>

#include <iostream>

#include "Fixtures.h"
#include "rbdl/rbdl_mathutils.h"
#include "rbdl/Logging.h"

#include "rbdl/Model.h"
#include "rbdl/Kinematics.h"
#include "rbdl/Dynamics.h"

using namespace std;
using namespace RigidBodyDynamics;
using namespace RigidBodyDynamics::Math;

const double TEST_PREC = 1.0e-14;

struct SphericalJoint {
	SphericalJoint () {
		ClearLogOutput();

		emulated_model.gravity = Vector3d (0., 0., -9.81); 
		spherical_model.gravity = Vector3d (0., 0., -9.81); 

		body = Body (1., Vector3d (1., 0., 0.), Vector3d (1., 1., 1.));

		joint_rot_zyx = Joint (
				SpatialVector (0., 0., 1., 0., 0., 0.),
				SpatialVector (0., 1., 0., 0., 0., 0.),
				SpatialVector (1., 0., 0., 0., 0., 0.)
				);
		joint_spherical = Joint (JointTypeSpherical);

		joint_rot_y = Joint (SpatialVector (0., 1., 0., 0., 0., 0.));

		emu_body_id = emulated_model.AddBody (0, Xtrans (Vector3d (0., 0., 0.)), joint_rot_zyx, body);
		emu_child_id = emulated_model.AppendBody (Xtrans (Vector3d (1., 0., 0.)), joint_rot_y, body);

		sph_body_id = spherical_model.AddBody (0, Xtrans (Vector3d (0., 0., 0.)), joint_spherical, body);
		sph_child_id = spherical_model.AppendBody (Xtrans (Vector3d (1., 0., 0.)), joint_rot_y, body);

		emuQ = VectorNd::Zero ((size_t) emulated_model.q_size);
		emuQDot = VectorNd::Zero ((size_t) emulated_model.qdot_size);
		emuQDDot = VectorNd::Zero ((size_t) emulated_model.qdot_size);
		emuTau = VectorNd::Zero ((size_t) emulated_model.qdot_size);

		sphQ = VectorNd::Zero ((size_t) spherical_model.q_size);
		sphQDot = VectorNd::Zero ((size_t) spherical_model.qdot_size);
		sphQDDot = VectorNd::Zero ((size_t) spherical_model.qdot_size);
		sphTau = VectorNd::Zero ((size_t) spherical_model.qdot_size);
	}

	Joint joint_rot_zyx;
	Joint joint_spherical;
	Joint joint_rot_y;
	Body body;

	unsigned int emu_body_id, emu_child_id, sph_body_id, sph_child_id;

	Model emulated_model;
	Model spherical_model;

	VectorNd emuQ;
	VectorNd emuQDot;
	VectorNd emuQDDot;
	VectorNd emuTau;

	VectorNd sphQ;
	VectorNd sphQDot;
	VectorNd sphQDDot;
	VectorNd sphTau;
};

TEST_FIXTURE(SphericalJoint, TestQIndices) {
	CHECK_EQUAL (0, spherical_model.mJoints[1].q_index);
	CHECK_EQUAL (3, spherical_model.mJoints[2].q_index);

	CHECK_EQUAL (4, emulated_model.q_size);
	CHECK_EQUAL (4, emulated_model.qdot_size);

	CHECK_EQUAL (5, spherical_model.q_size);
	CHECK_EQUAL (4, spherical_model.qdot_size);
	CHECK_EQUAL (4, spherical_model.spherical_w_index[1]);
}

TEST_FIXTURE(SphericalJoint, TestGetQuaternion) {
	spherical_model.AppendBody (Xtrans (Vector3d (1., 0., 0.)), joint_spherical, body);

	sphQ = VectorNd::Zero ((size_t) spherical_model.q_size);
	sphQDot = VectorNd::Zero ((size_t) spherical_model.qdot_size);
	sphQDDot = VectorNd::Zero ((size_t) spherical_model.qdot_size);
	sphTau = VectorNd::Zero ((size_t) spherical_model.qdot_size);

	CHECK_EQUAL (9, spherical_model.q_size);
	CHECK_EQUAL (7, spherical_model.qdot_size);

	CHECK_EQUAL (0, spherical_model.mJoints[1].q_index);
	CHECK_EQUAL (3, spherical_model.mJoints[2].q_index);
	CHECK_EQUAL (4, spherical_model.mJoints[3].q_index);

	CHECK_EQUAL (7, spherical_model.spherical_w_index[1]);
	CHECK_EQUAL (8, spherical_model.spherical_w_index[3]);

	sphQ[0] = 0.;
	sphQ[1] = 1.;
	sphQ[2] = 2.;
	sphQ[3] = 3.;
	sphQ[4] = -6.;
	sphQ[5] = -7.;
	sphQ[6] = -8.;
	sphQ[7] = 4.;
	sphQ[8] = -9.;

	Quaternion reference_1 (0., 1., 2., 4.);
	Quaternion quat_1 = spherical_model.GetQuaternion (1, sphQ);
	CHECK_ARRAY_EQUAL (reference_1.data(), quat_1.data(), 4);

	Quaternion reference_3 (-6., -7., -8., -9.);
	Quaternion quat_3 = spherical_model.GetQuaternion (3, sphQ);
	CHECK_ARRAY_EQUAL (reference_3.data(), quat_3.data(), 4);
}

TEST_FIXTURE(SphericalJoint, TestSetQuaternion) {
	spherical_model.AppendBody (Xtrans (Vector3d (1., 0., 0.)), joint_spherical, body);

	sphQ = VectorNd::Zero ((size_t) spherical_model.q_size);
	sphQDot = VectorNd::Zero ((size_t) spherical_model.qdot_size);
	sphQDDot = VectorNd::Zero ((size_t) spherical_model.qdot_size);
	sphTau = VectorNd::Zero ((size_t) spherical_model.qdot_size);

	Quaternion reference_1 (0., 1., 2., 3.);
	spherical_model.SetQuaternion (1, reference_1, sphQ);
	Quaternion test = spherical_model.GetQuaternion (1, sphQ);
	CHECK_ARRAY_EQUAL (reference_1.data(), test.data(), 4);

	Quaternion reference_2 (11., 22., 33., 44.);
	spherical_model.SetQuaternion (3, reference_2, sphQ);
	test = spherical_model.GetQuaternion (3, sphQ);
	CHECK_ARRAY_EQUAL (reference_2.data(), test.data(), 4);
}

TEST_FIXTURE(SphericalJoint, TestForwardDynamicsSimple) {
	emuQ[0] = 1.;
	emuQ[1] = 1.;
	emuQ[2] = 1.;
	emuQ[3] = 1.;

	for (unsigned int i = 0; i < emuQ.size(); i++) {
		sphQ[i] = emuQ[i];
	}

	Quaternion quat =  Quaternion::fromAxisAngle (Vector3d (1., 0., 0.), emuQ[2]) 
		* Quaternion::fromAxisAngle (Vector3d (0., 1., 0.), emuQ[1])
		* Quaternion::fromAxisAngle (Vector3d (0., 0., 1.), emuQ[0]);
	spherical_model.SetQuaternion (1, quat, sphQ);

	Matrix3d emu_orientation = CalcBodyWorldOrientation (emulated_model, emuQ, emu_child_id);
	Matrix3d sph_orientation = CalcBodyWorldOrientation (spherical_model, sphQ, sph_child_id);

	CHECK_ARRAY_CLOSE (emu_orientation.data(), sph_orientation.data(), 9, TEST_PREC);

	ForwardDynamics (emulated_model, emuQ, emuQDot, emuTau, emuQDDot);
	emuQDot = emuQDot + 0.001 * emuQDDot;
	emuQ = emuQ + 0.001 * emuQDot;
	
	ForwardDynamics (spherical_model, sphQ, sphQDot, sphTau, sphQDDot);
	VectorNd qderivative = spherical_model.GetQDerivative (sphQ, sphQDot);

	sphQDot = sphQDot + 0.001 * sphQDDot;
	sphQ = sphQ + 0.001 * spherical_model.GetQDerivative (sphQ, sphQDot);

	Quaternion new_quat = spherical_model.GetQuaternion (1, sphQ);
	new_quat.normalize();
	cout << new_quat.squaredNorm() << endl;
	spherical_model.SetQuaternion (1, new_quat, sphQ);

	emu_orientation = CalcBodyWorldOrientation (emulated_model, emuQ, emu_child_id);
	sph_orientation = CalcBodyWorldOrientation (spherical_model, sphQ, sph_child_id);

	CHECK_ARRAY_CLOSE (emu_orientation.data(), sph_orientation.data(), 9, TEST_PREC);

	cout << "emu = " << endl << emu_orientation << endl;
	cout << "emu = " << endl << emu_orientation << endl;
	cout << "Orientation error: " << endl << emu_orientation - sph_orientation << endl;
}
