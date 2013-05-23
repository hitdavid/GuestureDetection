#include "StdAfx.h"
#include "CandidateTracer.h"
#include "MovementInfo.h"
#include "MousePointerHelper.h"
#include "ImageProcessor.h"
#include <ctime>


Handjet::CandidateTracer::CandidateTracer(void) {
	m_lastFrameCandidates = new std::list<CandidateObject*>();
	m_thisFrameCandidates = new std::list<CandidateObject*>();
	m_tracingTable = new std::list<TracingTableItem*>();
	analyzer = new TransformAnalyzer();
	m_filter = new PositionSmoothFilter();
	isLeftDown = false;
	lastLeftDownTime = clock();
	lastLeftUpTime = clock();
	lastGuestureTime = clock();
	isDraging = false;
	mode1LastPoint = cvPoint(0, 0);
}


Handjet::CandidateTracer::~CandidateTracer(void)
{
	delete m_filter;
}

void Handjet::CandidateTracer::updateCandidates(std::list<CandidateObject*>* l) {
	CandidateObject* co = NULL;
	while ( m_lastFrameCandidates->size() > 0)
	{
		co = m_lastFrameCandidates->back();
		//hc->clearAllDefectBlocks();
		delete co;
		co = NULL;
		m_lastFrameCandidates->pop_back();
	}
	//delete m_lastFrameCandidates;
	for(std::list<CandidateObject*>::iterator it=m_thisFrameCandidates->begin(); it != m_thisFrameCandidates->end(); ++it) {
		m_lastFrameCandidates->push_back(*it);
	}

	m_thisFrameCandidates->clear();
	for(std::list<CandidateObject*>::iterator it=l->begin(); it != l->end(); ++it) {
		m_thisFrameCandidates->push_back(*it);
	}

	CandidateObject* hand = analyzer->analyze(m_lastFrameCandidates, m_thisFrameCandidates, m_tracingTable);

	if(hand != NULL) {

		int mode = ImageProcessor::Instance()->getRunMode();
		if(mode == 1) {
			//ppt mode
			if(mode1LastPoint.x == 0 && mode1LastPoint.y == 0) {
				mode1LastPoint = hand->getValidPosition();
			}
			else {
				//if(!CandidateObject::haveJudgeResultBit(hand->getShape(), CandidateObject::MAYBE_OPEN_PALM)) {
					
					clock_t  cts = clock() - lastGuestureTime;
					float timediff_sec = ((float)cts);// / CLOCKS_PER_SEC;
					if(timediff_sec > 3000) {
						CvPoint newPoint = hand->getValidPosition();

						if(newPoint.x - mode1LastPoint.x > 50) {
							MousePointerHelper::Instance()->nextSlideAction();
						}
						else if(newPoint.x - mode1LastPoint.x < -50) {
							MousePointerHelper::Instance()->prevSlideAction();
						}
						lastGuestureTime = clock();
					}			
				//}
				mode1LastPoint = hand->getValidPosition();
			}
			//return;
		}

		if(CandidateObject::haveJudgeResultBit(hand->getShape(), CandidateObject::MAYBE_FIST)) {
			if(!isLeftDown) {
				isLeftDown = true;
				lastLeftDownTime = clock();
			}
			else {
				clock_t  cts = clock() - lastLeftDownTime;
				float timediff_sec = ((float)cts);// / CLOCKS_PER_SEC;
				if(timediff_sec > 3000 && isDraging == false) {
					if(mode == 0) {
						MousePointerHelper::Instance()->dragBegin();
						
					}
					isDraging = true;
				}
				else if(isDraging) {
					
					clock_t  cts = clock() - lastLeftDownTime;
					float timediff_sec = ((float)cts);// / CLOCKS_PER_SEC;
					if(timediff_sec > 100) {
						lastLeftDownTime = clock();
						MovementInfo* i = m_filter->addOriginalPosition(hand->getValidPosition());
						CvPoint p = m_filter->retriveSmoothedPosition();
						MousePointerHelper::Instance()->MoveTo(p);
					}
					
				}
				else {
					//MovementInfo* i = m_filter->addOriginalPosition(hand->getValidPosition());
					hand->getValidPosition();
					//CvPoint p = m_filter->retriveSmoothedPosition();
					//MousePointerHelper::Instance()->MoveTo(p);
				}
			}
		}
		else {
			if(isLeftDown) {
				isLeftDown = false;
				if(isDraging) {
					clock_t  cts = clock() - lastLeftUpTime;
					float timediff_sec = ((float)cts);// / CLOCKS_PER_SEC;
					if(timediff_sec > 1000) {
						if(mode == 0) {
							MousePointerHelper::Instance()->dragRelease();
							
						}
						isDraging = false;
					}
					
				}
				else {
					clock_t cts = clock() - lastLeftDownTime;
					float timediff_sec = ((float)cts);// / CLOCKS_PER_SEC;
					if(timediff_sec > 2000) {
						if(mode == 0) {
							MousePointerHelper::Instance()->leftDBClick();
						}
					}else if(timediff_sec > 500) {
						if(mode == 0) {
							MousePointerHelper::Instance()->leftClick();
						}
						else if(timediff_sec > 1000){
							MousePointerHelper::Instance()->leftClick();
						}
					}
				}
			}
			else {
				if(CandidateObject::haveJudgeResultBit(hand->getShape(), CandidateObject::MAYBE_OPEN_PALM)) {
						MovementInfo* i = m_filter->addOriginalPosition(hand->getValidPosition());
						CvPoint p = m_filter->retriveSmoothedPosition();
						MousePointerHelper::Instance()->MoveTo(p);
				}
			}
		}
	}
	
}