#include "TCanvas.h"
#include "WaveformAnalysis.h"
#include "TH1.h"
#include <vector>
#include <cmath>
#include <iostream>

using namespace std;

#define max(x,y) x>y?x:y
#define min(x,y) x<y?x:y
#define hget(h,i) h->GetBinContent(i)
#define hcenter(h,i) h->GetXaxis()->GetBinCenter(i)
#define verase(v,i) v.erase(v.begin()+i)

double WaveformAnalysis::baseline(const TH1* hist, double& rms, int binMin, int binMax)
{

  double base=0;
  rms=0;
  
  int n = binMax-binMin+1;
  for (int i = binMin; i<=binMax; i++) base+=hget(hist,i);
  base/=n;
  
  for (int i = binMin; i<=binMax; i++) rms+=pow(hget(hist,i)-base,2);
  rms/=(n-1);
  rms = sqrt(rms);
  return base;

}

int WaveformAnalysis::minbin(const TH1* hist)
{
  int minbin=0;
  double minval = DBL_MAX; 
  // neglects the underflow, the overflow, and the last bin

  for (int i = 1; i< hist->GetNbinsX(); i++)
  {
    if (hist->GetBinContent(i) < minval)
    {
      minval = hist->GetBinContent(i); 
      minbin = i; 
    }
  }
  
  return minbin;
} 

int WaveformAnalysis::maxbin(const TH1* hist)
{
  int maxbin=0;
  double maxval = -DBL_MAX; 
  // neglects the underflow, the overflow, and the last bin
  for (int i = 1; i< hist->GetNbinsX(); i++)
  {
    if (hist->GetBinContent(i) > maxval)
    {
      maxval = hist->GetBinContent(i); 
      maxbin = i; 
    }
  }
 
  return maxbin;
}

double WaveformAnalysis::integral(const TH1* hist, double start, double end, double ped=0, bool doWeight=false)
{
  start = max(hist->GetXaxis()->GetXmin(),start);
  end = min(hist->GetXaxis()->GetXmax(),end);
  int startBin = hist->GetXaxis()->FindBin(start);
  int endBin = hist->GetXaxis()->FindBin(end);

  double startCenter = hcenter(hist,startBin);
  double endCenter = hcenter(hist,endBin);
  
  double width = hist->GetXaxis()->GetBinWidth(1);

  double sum = 0;
  
  double startBinFrac = (start-startCenter+0.5*width)/width;
  double endBinFrac = (endCenter - 0.5*width-end)/width;
  
  if (doWeight)
  {
	  startBinFrac*=(double)startBin;
	  endBinFrac*=(double)endBin;
  }
  
  if (ped>0) 
  {
	  sum += TMath::Abs(ped-hget(hist,startBin)) * startBinFrac;
	  sum += TMath::Abs(ped-hget(hist,endBin)) * endBinFrac;
	  for(int i = startBin+1; i<endBin; i++) 
	  {
		  double w = doWeight?i:1.;
		  sum += w*TMath::Abs(ped-hget(hist,i));
	  }
  }
  else
  {
	  sum += hget(hist,startBin) * startBinFrac;
	  sum += hget(hist,endBin) * endBinFrac;
	  for(int i = startBin+1; i<endBin; i++)
	  {
		  double w = doWeight?i:1.;
		  sum += w*(hget(hist,i));
	  }
  }
  
  return sum;
}

vector<int> WaveformAnalysis::peaks(const TH1* hist, double threshold, int minBin,int maxBin)
{

  minBin = minBin<0?1:minBin;
  maxBin = maxBin<0?hist->GetNbinsX():maxBin;
  vector<int> p;
  double v, lastV=hget(hist,minBin);
  bool isInPeak = false;
  for (int i = minBin; i<= maxBin; i++)
  {

    v = hget(hist,i);
    if (lastV<threshold&&v>=threshold){
      isInPeak=true;
      p.push_back(i);
    }else if (lastV>= threshold&&v<threshold) isInPeak=false;

    if (isInPeak){
      if (v>hget(hist,p[p.size()-1])) p[p.size()-1] = i;
    }

    lastV=v;
  }
  return p;

}

vector<int> WaveformAnalysis::valleys(const TH1* hist, double threshold, int minBin,int maxBin)
{

  minBin = minBin<0?1:minBin;
  maxBin = maxBin<0?hist->GetNbinsX():maxBin;
  vector<int> p;
  double v, lastV=hget(hist,minBin);
  bool isInPeak = false;
  for (int i = minBin; i<= maxBin; i++)
  {

    v = hget(hist,i);
    if (lastV>threshold&&v<=threshold){
      isInPeak=true;
      p.push_back(i);
    }else if (lastV<= threshold&&v>threshold) isInPeak=false;
    
    if (isInPeak){
      if (v<hget(hist,p[p.size()-1])) p[p.size()-1] = i;
    }
    
    lastV=v;
  }
  return p;

}

/*
double WaveformAnalysis::calc_S1_charge(TH1* hist, double ped, double tau)
{
	double charge=0.;
	double delta_tau_start=0.8;
	double delta_tau_end=50.;
	
	TH1F* h = (TH1F*)hist->Clone("htmp");
	
	cout << "ped = " << ped << endl;
	
	TF1* fit = new TF1("fit","[0]-[1]*exp(-[2]/(x-[3]))",tau,tau+100.);
	fit->SetParameter(0,ped);
	fit->SetParameter(3,tau);
	fit->SetParLimits(0,0.95*ped,4096);
	fit->SetParLimits(3,tau-0.1,tau+0.1);
	TFitResultPtr res2 = h->Fit(fit,"RMS","",tau+delta_tau_start,tau+delta_tau_end);
	
	// drawing
	TCanvas *c2 = new TCanvas("c2","c2",1000,1000);
	c2->Divide(2,2);
	c2->cd(2);
	h->GetXaxis()->SetRange(h->FindBin(tau+0.5),h->FindBin(tau+delta_tau_end+1.));
	h->Draw("hist");
	fit->SetLineColor(kRed);
	fit->Draw("same");
	
	TLine lped,l1;
	
	c2->Update();
	lped = TLine(gPad->GetUxmin(),ped,gPad->GetUxmax(),ped);
	lped.SetLineColor(kBlack);
	lped.SetLineStyle(2);
	lped.Draw("same");
	l1 = TLine(tau+1.,gPad->GetUymin(),tau+1,gPad->GetUymax());
	l1.SetLineColor(kBlack);
	l1.SetLineStyle(2);
	l1.Draw("same");
	c2->Modified();
	c2->Update();
	
	c2->cd(1);
	hist->GetXaxis()->SetRange(hist->FindBin(tau+0.5),hist->FindBin(tau+50));
	hist->Draw("hist");
	fit->Draw("same");
	c2->Modified();
	c2->Update();


	h->GetXaxis()->SetRange(0,0);
	
	

	charge = integral(h,tau-0.05,tau+0.95,ped);
	double charge2 = integral(h,tau+0.95,tau+delta_tau_end,ped);
	
	TF1* fit_line = new TF1("fit_line","[0]",tau,tau+delta_tau_end);
	fit_line->SetParameter(0,res2->Parameter(0));
	//fit_line->SetParameter(1,res2->Parameter(1));
	//fit_line->SetParameter(2,res2->Parameter(4));
	TF1* fit_expo = new TF1("fit_expo","[0]*exp(-[1]/(x-[2]))",tau,tau+delta_tau_end);
	fit_expo->SetParameter(0,fit->GetParameter(1));
	fit_expo->SetParameter(1,fit->GetParameter(2));
	fit_expo->SetParameter(2,fit->GetParameter(3));
	
	double int_line = fit_line->Integral(tau+0.95,tau+delta_tau_end)/4.E-3;
	double int_expo = fit_expo->Integral(tau+0.95,tau+delta_tau_end)/4.E-3;
	
	printf("charge = %0.3e, charge2 = %0.3e, int_line = %0.3e, int_expo = %0.3e\n",charge,charge2,int_line,int_expo);	
	
	c2->cd(3);
	fit_line->Draw();
	c2->cd(4);
	fit_expo->Draw();
	
	c2->Modified();
	c2->Update();
	
	lets_pause();
		
	delete fit_line;
	delete fit_expo;
	delete fit;
	delete h;

	return charge;
}
*/

double WaveformAnalysis::calc_S1_charge_m2(const TH1* hist, double ped, int binpeak, int &endbin)
{
	endbin=0;

	for (int i=binpeak; i<hist->GetNbinsX()+1; i++) 
	{
		//if ((ped-hget(h,i))<0.02*halfamp) { S1_end2=i; break; }
		if (hget(hist,i)>ped) { endbin=i; break; }
	}
	
	double tmin=hcenter(hist,binpeak);
	
	return integral(hist,tmin-0.05,hcenter(hist,endbin),ped);
}
		
	
double WaveformAnalysis::calc_S1_parameters(const TH1* hist, double ped, int &binpeak, double &width, int minbin=0, int maxbin=0)
{
	binpeak=0;
	width=0;
	double charge=0;
	
	gStyle->SetOptStat(0);
    gStyle->SetPadTickX(1);
    gStyle->SetPadTickY(1);
	
	TH1F* h = (TH1F*)hist->Clone("htmp");
	
	if (!minbin && !maxbin) cout << "calc_S1_parameters::Warning: Using full range for S1 calculation" << endl;
	else h->GetXaxis()->SetRange(minbin,maxbin);
	
	binpeak=h->GetMinimumBin();
	
	
	/*
	TCanvas* c1 = new TCanvas("c1","c1",1200,400);
	c1->Divide(3,1);

	c1->cd(1);
	h->Draw("hist");
	
	double tmin=hcenter(h,binpeak);
	double tmin_min = tmin-0.02;
	double tmin_max1 = tmin+0.06;
	double tmin_max2 = tmin+0.08;
	double tmin_max3 = tmin+0.980;
	double tmin_max4 = tmin+3.980;
	
	TLine ll0,l0,l1,l2,l3,l4;
	
	c1->Update();
	ll0 = TLine(gPad->GetUxmin(),ped,gPad->GetUxmax(),ped);
	ll0.SetLineColor(kRed);
	ll0.Draw("same");
	l0 = TLine(tmin_min,gPad->GetUymin(),tmin_min,gPad->GetUymax());
	l0.SetLineColor(kRed);
	l0.Draw("same");
	l1=TLine(tmin_max1,gPad->GetUymin(),tmin_max1,gPad->GetUymax());
	l1.SetLineColor(kOrange-7);
	l1.Draw("same");
	l2=TLine(tmin_max2,gPad->GetUymin(),tmin_max2,gPad->GetUymax());
	l2.SetLineColor(kGreen+3);
	l2.Draw("same");
	l3=TLine(tmin_max3,gPad->GetUymin(),tmin_max3,gPad->GetUymax());
	l3.SetLineColor(kOrange+8);
	l3.Draw("same");
	l4=TLine(tmin_max4,gPad->GetUymin(),tmin_max4,gPad->GetUymax());
	l4.SetLineColor(kViolet);
	l4.Draw("same");
	
	c1->cd(2);
	TH1F* htmp = (TH1F*)h->Clone("htmp");
	htmp->GetXaxis()->SetRange(htmp->FindBin(tmin-0.05),htmp->FindBin(tmin+0.14));
	htmp->Draw("hist");
	c1->Update();
	ll0 = TLine(gPad->GetUxmin(),ped,gPad->GetUxmax(),ped);
	ll0.SetLineColor(kRed);
	ll0.Draw("same");
	l0 = TLine(tmin_min,gPad->GetUymin(),tmin_min,gPad->GetUymax());
	l0.SetLineColor(kRed);
	l0.Draw("same");
	l1=TLine(tmin_max1,gPad->GetUymin(),tmin_max1,gPad->GetUymax());
	l1.SetLineColor(kOrange-7);
	l1.Draw("same");
	l2=TLine(tmin_max2,gPad->GetUymin(),tmin_max2,gPad->GetUymax());
	l2.SetLineColor(kGreen+3);
	l2.Draw("same");
	l3=TLine(tmin_max3,gPad->GetUymin(),tmin_max3,gPad->GetUymax());
	l3.SetLineColor(kOrange+8);
	l3.Draw("same");
	l4=TLine(tmin_max4,gPad->GetUymin(),tmin_max4,gPad->GetUymax());
	l4.SetLineColor(kViolet);
	l4.Draw("same");
	
	
	c1->cd(3);
	TH1F* hint = (TH1F*)h->Clone("hint");
	double t0=hcenter(h,minbin);
	double tmax=hcenter(h,maxbin);
	double totcharge=integral(h,t0,tmax,ped);
	for (int i=minbin; i<=maxbin; i++)
	{
		hint->SetBinContent(i,(1./totcharge)*integral(h,t0,hcenter(hint,i),ped));
	}
	hint->GetYaxis()->SetTitle("Fraction of 'total' charge");
	hint->Draw("hist");
	c1->Update();
	ll0 = TLine(gPad->GetUxmin(),ped,gPad->GetUxmax(),ped);
	ll0.SetLineColor(kRed);
	ll0.Draw("same");
	l0 = TLine(tmin_min,gPad->GetUymin(),tmin_min,gPad->GetUymax());
	l0.SetLineColor(kRed);
	l0.Draw("same");
	l1=TLine(tmin_max1,gPad->GetUymin(),tmin_max1,gPad->GetUymax());
	l1.SetLineColor(kOrange-7);
	l1.Draw("same");
	l2=TLine(tmin_max2,gPad->GetUymin(),tmin_max2,gPad->GetUymax());
	l2.SetLineColor(kGreen+3);
	l2.Draw("same");
	l3=TLine(tmin_max3,gPad->GetUymin(),tmin_max3,gPad->GetUymax());
	l3.SetLineColor(kOrange+8);
	l3.Draw("same");
	l4=TLine(tmin_max4,gPad->GetUymin(),tmin_max4,gPad->GetUymax());
	l4.SetLineColor(kViolet);
	l4.Draw("same");

	c1->Modified();
	c1->Update();
	
	lets_pause();
	*/
	
	h->GetXaxis()->SetRange(0,0);
	
	double halfamp = 0.5*(ped-hget(h,binpeak));
	
	// find bins corresponding to FWHM
	int S1_start=0;
	int S1_end=0;
	for (int i=binpeak; i<h->GetNbinsX()+1; i++) 
	{
		if ((ped-hget(h,i))<halfamp) { S1_end=i; break; }
	}

	for (int i=binpeak; i>0; i--) 
	{
		if ((ped-hget(h,i))<halfamp) { S1_start=i; break; }
	}
	
	// interpolate to find fraction of x bin width where y = halfamp
	double y1=ped-hget(h,S1_start);
	double y2=ped-hget(h,S1_start+1);
	double deltaX = h->GetBinWidth(S1_start);
	double m = y2-y1;
	
	double deltax_start = (y2-halfamp)/m;
		
	y1=ped-hget(h,S1_end);
	y2=ped-hget(h,S1_end-1);
	m=y2-y1;
	
	double deltax_end = (y2-halfamp)/m;
	
	width=(S1_end-1)-(S1_start+1)+deltax_start+deltax_end;
	
	// now integrate to find charge between -50 ns and 3.95 us
	double tmin=hcenter(h,binpeak);
	charge = integral(h,tmin-0.05,tmin+3.95,ped);
	
	delete h;

	return charge;
}


double WaveformAnalysis::calc_S2_parameters(const TH1* hist, double ped, int binpeak_S1, int &binpeak_S2, int &binavg_S2)
{
	binpeak_S2=0;
	binavg_S2=0;
	double d_binavg_S2=0;
	double charge=0;
	
	gStyle->SetOptStat(0);
    gStyle->SetPadTickX(1);
    gStyle->SetPadTickY(1);
	
	TH1F* h = (TH1F*)hist->Clone("htmp");
	
	double tmin_S1 = hcenter(h,binpeak_S1);

	double tstart_S2 = tmin_S1+3.95;
	int startbin = h->FindBin(tstart_S2);
	
	//int endbin = h->GetNbinsX()+1;
	double tend_S2 = 900.; // h->GetBinLowEdge(endbin)+h->GetBinWidth(endbin);
	int endbin = h->FindBin(tend_S2);
	
	h->GetXaxis()->SetRange(startbin,endbin-1);
	
	binpeak_S2=h->GetMinimumBin();
	
	//printf("startbin = %d, endbin = %d, tstart_S2 = %f, tend_S2 = %f\n",startbin,endbin,tstart_S2,tend_S2);
	
	charge = integral(h,tstart_S2,tend_S2,ped);
	d_binavg_S2 = integral(h,tstart_S2,tend_S2,ped,true);
    d_binavg_S2/=charge;
	binavg_S2=(int)d_binavg_S2; 
	
	/*
	TF1* fit = new TF1("fit","pol1",800,1100);
	hist->Fit(fit,"R","",900,1048);
	TCanvas *c1 = new TCanvas("c1","c1");
	hist->Draw();
	c1->Modified();
	c1->Update();
	lets_pause();
	*/
	
	delete h;
	
	return charge;
}