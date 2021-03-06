// PasswordTesting.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "UserList.h"

#include <cryptlib.h>
#include <filters.h>
#include <sha.h>
#include <base64.h>
#include <files.h>
#include <modes.h>
#include <aes.h>

#include <iostream>
#include <string>

using namespace std;
using namespace CryptoPP;


/***********
 * Globals *
 ***********/
SHA chosen_hash;
extern User *UserList;

#define AES_KEY_LEN AES::DEFAULT_KEYLENGTH
const byte aeskey[] = "HelloThereOneTwo"; // 16 characters = 128 bits


void LoadUsers()
{
	User *LastUser = FindLastUser();

	string fullfile;
	CTR_Mode<AES>::Decryption decrypt(aeskey, AES_KEY_LEN, aeskey); // Using the key as the initialization vector
	FileSource infile("passwords.txt", true, new Base64Decoder(new StreamTransformationFilter(decrypt, new StringSink(fullfile))), false); // decrypt the file while importing it

	// Parse the decrypted file
	string tempstr;
	StringSource filestr(fullfile, false, new StringSink(tempstr)); // false = do not read the string yet
	while (1)
	{
		// Get the next username
		tempstr.clear();
		if (filestr.Pump(MAX_NAME_LENGTH) <= 0) // Make sure that something was pumped
			break;

		// Strip padding from the username and copy it into the new string
		for (size_t x = tempstr.length() - 1; x > 0; x--)
			if (tempstr[x] == '~')
				tempstr[x] = 0;
		string username = tempstr;

		// Get the next password
		tempstr.clear();
		filestr.Pump(PASSWORD_LENGTH);

		// Put the user in the list
		MakeUser(username,tempstr);

		tempstr.clear();
		filestr.Pump(2); // Strip the \r\n
	}
}

void SaveUsers()
{
	CTR_Mode<AES>::Encryption encrypt(aeskey, AES_KEY_LEN, aeskey); // Using the key as the initialization vector
	StreamTransformationFilter outfile(encrypt, new Base64Encoder(new FileSink("passwords.txt", false)));

	for (User *CurUser = UserList; CurUser != NULL; CurUser = CurUser->Next)
	{
		// Put together the output for each user
		string userstr = CurUser->Name;
		for (int x = userstr.length(); x < MAX_NAME_LENGTH; x++)
			userstr += "~"; // Add padding
		userstr += CurUser->PasswordHash;
		userstr += "\r\n";
		
		// Write to the encrypted output file
		outfile.Put((const byte*)userstr.c_str(), userstr.length());
	}
}

void TestUser()
{
	// Get the username and password
	string username, password;
	cout << "Enter user name: ";
	cin >> username;
	User *TheUser = FindUser(username);
	if (TheUser == NULL)
	{
		cout << "That user doesn't exist" << endl;
		return;
	}
	cout << "Enter a password: ";
	cin >> password;

	// Decode the hashed password and concatenate with the value to test
	string teststring = password;
	StringSource(TheUser->PasswordHash, true, new Base64Decoder(new StringSink(teststring)));

	// Test the entered value with the hash
	byte result[1];
	word32 flags = HashVerificationFilter::HASH_AT_END|HashVerificationFilter::PUT_RESULT;
	StringSource(teststring, true, new HashVerificationFilter(chosen_hash, new ArraySink(result, sizeof(result)), flags));

	if (result[0] != 0)
		cout << "CORRECT" << endl;
	else
		cout << "WRONG" << endl;

	cout << endl;
}

void AddUser()
{
	// Get the username and password
	string username, password;
	cout << "Enter user name: ";
	cin >> username;
	if (FindUser(username) != NULL)
	{
		cout << "That user already exists" << endl;
		return;
	}
	cout << "Enter a password: ";
	cin >> password;

	// Generate the encoded hash of the password
	string hash;
	StringSource(password, true, new HashFilter(chosen_hash, new Base64Encoder(new StringSink(hash), false)));

	cout << username << "::" << hash << endl;
	cout << " Username Length: " << username.length() << endl;
	cout << " Password Length: " << hash.length() << endl;

	// Add the user to the list
	MakeUser(username,hash);

	cout << endl;
}

void ChangeUser()
{
}

void ViewUsers()
{
	cout << "Current Users:" << endl;
	cout << "--------------" << endl;
	for (User *CurUser = UserList; CurUser != NULL; CurUser = CurUser->Next)
	{
		cout << CurUser->Name << " :: " << CurUser->PasswordHash << endl;
	}
	cout << endl;
}

int _tmain(int argc, _TCHAR* argv[])
{
	// Initialization
	UserList = NULL;
	LoadUsers();

	// User options
	bool done = false;
	while (!done)
	{
		done = false;
		cout << "Available options:" << endl;
		cout << "\t1) Test a username/password" << endl;
		cout << "\t2) Add a username/password" << endl;
		cout << "\t3) Change a username/password" << endl;
		cout << "\t4) View current usernames" << endl;
		cout << "\t5) Quit" << endl;
		cout << "Enter your choice: ";

		int choice = 0;
		cin >> choice;
		cout << endl;
		switch(choice)
		{
		case 1: TestUser(); break;
		case 2: AddUser(); break;
		case 3: ChangeUser(); break;
		case 4: ViewUsers(); break;
		case 5: done = true; break;
		default:
			cout << "Enter a valid value (1-4)" << endl;
			break;
		}
	}

	SaveUsers();

	return 0;
}

