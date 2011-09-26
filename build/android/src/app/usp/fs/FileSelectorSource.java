/*
Portable ZX-Spectrum emulator.
Copyright (C) 2001-2011 SMT, Dexus, Alone Coder, deathsoft, djdron, scor

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

package app.usp.fs;

import java.io.File;
import java.io.InputStream;
import java.net.URL;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

abstract class FileSelectorSource
{
	class Item
	{
		Item() {}
		Item(final String _name) { name = _name; }
		String name;
		String desc;
		String url;
	};
	abstract public boolean GetItems(final File path, List<Item> items);
	abstract public boolean ApplyItem(Item item);
}

abstract class FSSWeb extends FileSelectorSource
{
	abstract String Root();
	abstract String BaseURL();
	abstract String[] Items2();
	abstract String[] Items2URLs();
	abstract String[] Patterns();
	abstract void Get(List<Item> items, Matcher m, final String _url, final String _name);
	public boolean GetItems(final File path, List<Item> items)
	{
		File path_up = path.getParentFile();
		if(path_up == null)
		{
			items.add(new Item(Root()));
			return false;
		}
		File r = path;
		for(;;)
		{
			File p = r.getParentFile();
			if(p.getParentFile() == null)
				break;
			r = p;
		}
		if(!r.toString().equals(Root()))
			return false;
		items.add(new Item("/.."));
		if(path_up.getParent() == null)
		{
			for(String i : Items2())
			{
				items.add(new Item(i));
			}
			return true;
		}
		int idx = 0;
		String n = "/" + path.getName().toString();
		for(String i : Items2())
		{
			if(i.equals(n))
			{
				ParseURL(Items2URLs()[idx], items, n);
				break;
			}
			++idx;
		}
		return true;
	}
	private void ParseURL(String _url, List<Item> items, final String _name)
	{
		try
		{
			URL url = new URL(BaseURL() + _url + ".htm");
			Charset charset = Charset.forName("windows-1251");
			CharsetDecoder decoder = charset.newDecoder();
			InputStream is = url.openStream();
			byte buffer[] = new byte[16384];
			String s = "";
			int r = 0;
			while((r = is.read(buffer)) != -1)
			{
				CharBuffer cb = decoder.decode(ByteBuffer.wrap(buffer, 0, r));
				s += cb;
			}
			is.close();
			for(String p : Patterns())
			{
				Pattern pt = Pattern.compile(p);
				Matcher m = pt.matcher(s);
				boolean ok = false;
				while(m.find())
				{
					ok = true;
					Get(items, m, _url, _name);
				}
				if(ok)
					break;
			}
		}
		catch(Exception e)
		{
		}
	}
}