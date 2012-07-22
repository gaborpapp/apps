/*                                                                           
 *      _____  __  _____  __     ____                                   
 *     / ___/ / / /____/ / /    /    \   FieldKit
 *    / ___/ /_/ /____/ / /__  /  /  /   (c) 2010, FIELD. All rights reserved.              
 *   /_/        /____/ /____/ /_____/    http://www.field.io           
 *   
 *	 Created by Marcus Wendt on 15/06/2010.
 */

#include "fieldkit/fbx/SceneController.h"
#include "fieldkit/Logger.h"

using namespace fieldkit::fbx;

SceneController::SceneController()
{
	animIndex = 0;
	cameraIndex = 0;
}

SceneController::SceneController( Scene* scene )
{
	animIndex = 0;
	cameraIndex = 0;
	setScene(scene);
}

void SceneController::setAnimation(int index) {
	scene->setAnimation(index);
	animIndex = index;
}

void SceneController::nextAnimation()
{
	animIndex++;
	if(animIndex >= scene->animationNames.GetCount())
		animIndex = 0;
	setAnimation(animIndex);
}

void SceneController::setCamera(int index)
{
	LOG_INFO( "SceneController::setCamera("<< index <<")");

	cameraIndex = index;
	KFbxGlobalSettings& settings = scene->fbxScene->GetGlobalSettings();
	char* name = (char *)scene->cameras[index]->GetName();
	settings.SetDefaultCamera(name);
}

void SceneController::nextCamera()
{
	cameraIndex++;
	if(cameraIndex >= scene->cameras.GetCount())
		cameraIndex = 0;
	setCamera(cameraIndex);
}

void SceneController::update() {
	//printf("SceneController::update start %f stop %f \n", scene->start.GetSecondDouble(), scene->stop.GetSecondDouble());

	// Loop in the animation stack.
	if (scene->stop > scene->start) {
		scene->currentTime += scene->period;

		//printf("Scene::update %f \n", scene->currentTime.GetSecondDouble());

		if (scene->currentTime > scene->stop) {
			scene->currentTime = scene->start;
		}
	}
}

void SceneController::setSpeed( float ms )
{
	scene->period.SetMilliSeconds( (long) ms );
}

float SceneController::getSpeed()
{
	return (float)scene->period.GetMilliSeconds();
}
