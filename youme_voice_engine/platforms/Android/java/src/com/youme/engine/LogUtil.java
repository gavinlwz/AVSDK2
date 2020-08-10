package com.youme.engine;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;

public class LogUtil {
	public static void SaveLogcat(String strPath)
	{
		try {  
		      Process process = Runtime.getRuntime().exec("logcat -v time -d ");  
		      BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(process.getInputStream()));  
		      		      
		      BufferedWriter file = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(strPath)));		      
		      String line;  
		      while ((line = bufferedReader.readLine()) != null) {  
		    	  file.write(line);
		    	  file.write("\n");
		      }  
		      file.close();
		    } catch (Exception e) {  
		    }  
	}
}
