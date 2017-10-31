#include <iostream>
#include <opencv2/core.hpp>
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

using namespace cv;
using namespace std;

void imgproc(Mat);
void redDetect(Mat);
void testHSV(Mat);
void recognize(Mat);

int main(int argc, char* argv[]) {
	VideoCapture cap; 
	Mat frame;
	char key = ' ';
	int i=0;
	while (!cap.open(i) && i <= 10) { i++; }
	if (!cap.open(i)) {
		frame = imread("temp.png");
		frame.convertTo(frame, -1, 1, -50);

		cout << "Press 'c' for method 1 and 'v' for method 2, 'q' to exit" << endl;
		cin >> key;	
		if (key == 'q') return 0;
		else if (key == 'c') imgproc(frame);
		else if (key == 'v') redDetect(frame);

		waitKey(0);
		return 0;
	}

	while (cap.open(i)) {
		cap.read(frame);
		
		namedWindow("Video", WINDOW_AUTOSIZE);
		imshow("Video",frame);

	if (argc == 2 && string(argv[1]) == "test") {
		testHSV(frame);
		return 0;
	}

		//cin >> key;
		//if (key == 'q') break;
		//else if (key == 'c') imgproc(frame);
		//else if (key == 'v') 
		redDetect(frame);
	
		waitKey(0);
	}

	return 0;
}

void imgproc(Mat frame) {
	Mat gray, thres, blr, can, cropf;
	vector< vector<Point> > contours, contours2, contoursR;
	
	cvtColor(frame, gray, COLOR_BGR2GRAY);
	GaussianBlur(gray, gray, Size(3,3), 1, 1);
	
	threshold(gray, gray, 60, 255, 0);
	
	Canny(gray, can, 0, 100, 3);
	findContours(can, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

	Mat drawing1 = Mat::zeros( frame.size(), CV_8UC1 ), drawing2;
  	for(int i=0; i<contours.size(); i++ ) 
  		if (contourArea(contours[i], false) > 2200 && contourArea(contours[i], false) < 3000)
  			drawContours( drawing1, contours, i, Scalar(255), FILLED, 8);
    
    Mat element = getStructuringElement(0,Size(10,10),Point(-1,-1));
    dilate(drawing1,drawing2,element);

 	findContours(drawing2, contours2, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

 	try {
 		if (contours2.size() > 0) {
		 	Rect cropR = boundingRect(Mat(contours2[0]));
		   	Mat recog = Mat::zeros(cropR.size() ,CV_8UC1);
		   	drawing1(cropR).copyTo(recog);
			rectangle(frame, cropR.tl(), cropR.br(), Scalar(0, 255,0), 2, 8, 0);
			
			recognize(recog);
		}
	}
	catch (int e) {
		cout << "Nothing recognized." << endl;
		return;
	}

	imshow("Original", frame);

	return;
}

void redDetect(Mat frame) {
	Mat frameH, thres;
	vector< vector<Point> > contours, contoursR;

	cvtColor(frame, frameH, COLOR_BGR2HSV);
	inRange(frameH, Scalar(0,120,150), Scalar(10,255,255), thres);
	//imshow("thresed", thres);

	Mat drawing1;
	Mat element = getStructuringElement(0,Size(10,10),Point(-1,-1));
    dilate(thres,drawing1,element);

    findContours(drawing1,contours,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE);

    double max=0, index=-1;
    for (int i=0; i<contours.size(); i++) {
    	if (contourArea(contours[i]) > max)	index = i;
    }

    try {
    	if (index != -1) {
		    Rect cropR = boundingRect(Mat(contours[index]));
		   	Mat recog = Mat::zeros(cropR.size() ,CV_8UC1);
		   	thres(cropR).copyTo(recog);
		   	rectangle(frame, cropR.tl(), cropR.br(), Scalar(0, 255,0), 2, 8, 0);
			
			recognize(recog);
		}
	}
	catch (int e) {
		cout << "Nothing recognized." << endl;
		return;
	}

	imshow("Original", frame);	

	return;
}

void testHSV(Mat frame) {
		Mat frameH, thres;
		int iLowH=0,iHighH=10,iLowS=120,iHighS=255,iLowV=150,iHighV=255;

		cvtColor(frame, frameH, COLOR_BGR2HSV);

		namedWindow("HSV Control",WINDOW_AUTOSIZE);
		cvCreateTrackbar("LowH", "HSV Control", &iLowH, 179);
		cvCreateTrackbar("HighH", "HSV Control", &iHighH, 179);
		cvCreateTrackbar("LowS", "HSV Control", &iLowS, 255);
		cvCreateTrackbar("HighS", "HSV Control", &iHighS, 255);
		cvCreateTrackbar("LowV", "HSV Control", &iLowV, 255);
		cvCreateTrackbar("HighV", "HSV Control", &iHighV, 255);
		
		while (1) {
		inRange(frameH, Scalar(iLowH,iLowS,iLowV), Scalar(iHighH,iHighS,iHighV), thres);
		imshow("thresed", thres);

		waitKey(10);
		}

		return;
}

void recognize(Mat recog) {
	vector< vector<Point> > contoursR;
	vector<Point> points;

	findNonZero(recog, points);
	RotatedRect box = minAreaRect(points);
	double angle = box.angle;
	if (angle > 45) angle -= 90;
	if (angle < -45) angle += 90;
	Mat rotM = getRotationMatrix2D(box.center, angle, 1);
	warpAffine(recog, recog, rotM, recog.size(), INTER_CUBIC);
	
    erode(recog,recog,getStructuringElement(0,Size(10,10),Point(-1,-1)));
    dilate(recog,recog,getStructuringElement(0,Size(2,2),Point(-1,-1)));

	GaussianBlur(recog, recog, Size(9,9), Point(-1,-1));

	findContours(recog, contoursR, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
  	vector<Rect> boundRect( contoursR.size() );

  	int cntRect=0;
	for(int i=0; i<contoursR.size(); i++ ) {
    	if (contourArea(contoursR[i]) > 50) {
    		//cout << i << " " << contourArea(contoursR[i]) << endl;
    		boundRect[cntRect] = boundingRect( Mat(contoursR[i]) );
       		rectangle( recog, boundRect[cntRect].tl(), boundRect[cntRect].br(), Scalar(200, 200,255), 2, 8, 0 );
			cntRect++;
		}
	}

	namedWindow("recognize", WINDOW_AUTOSIZE);
	imshow("recognize", recog);

	int cnt_pos = 0;
	int width=recog.cols, height=recog.rows;
	//cout << height << " " << width << endl;

	int strokeTL=0, strokeBL=0, strokeL=0, strokeR=0;

	switch (cntRect) {
		case 2: cout << 1 << endl;
				break;
		case 3: cout << 7 << endl;
				break;
		case 4: cout << 4 << endl;
				break;
		case 5: for (int i=0; i<cntRect; i++) {
					if (boundRect[i].tl().x < width/2 && boundRect[i].br().x < width/2)
						if (boundRect[i].br().y < height*2/3) strokeTL++;
						else strokeBL++;
				}
				if (strokeTL == 1) cout << 5 << endl;
				else if (strokeBL == 1) cout << 2 << endl;
				else cout << 3 << endl;
				break;
		case 6: for (int i=0; i<cntRect; i++) {
					if (boundRect[i].tl().x < width/2 && boundRect[i].br().x < width/2) strokeL++;
					if (boundRect[i].tl().x > width/2 && boundRect[i].br().x > width/2) strokeR++;
				}
				if (strokeL == 1) cout << 9 << endl;
				else if (strokeR == 1) cout << 6 << endl;
				else cout << 0 << endl;
				break;
		case 7: cout << 8 << endl;
				break;
		default: cout << 0 << endl;
				 break;

	}

	return;
}