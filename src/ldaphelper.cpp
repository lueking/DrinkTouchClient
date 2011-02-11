#include "ldaphelper.h"
#include <QMessageBox>
#include <QDebug>

LdapHelper::LdapHelper(QString userDN, QString password, QString address) {
    this->address = address;
    this->connected = false;
    this->userDN = userDN;
    this->password = password;
}

LdapHelper::~LdapHelper() {
    if (connected) {
        disconnect();
    }
}

bool LdapHelper::connect() {
    QString connectionString = address;
    int error = ldap_initialize(&ld, connectionString.toAscii().data());
    if (error != LDAP_SUCCESS) {
        QMessageBox::critical(0, "openLDAP", QString("Failed to initialize (error: %1)").arg(error));
        return false;
    }
    qDebug() << "Initialized " << connectionString << error;

    // set the LDAP version to be 3
    int version = LDAP_VERSION3;
    error = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version);
    if (error != LDAP_OPT_SUCCESS)
    {
        QMessageBox::critical(0, "openLDAP", QString("Failed to set options (error: %1)").arg(error));
        return false;
    }
    qDebug() << "Set options" << connectionString << error;

    //error = ldap_simple_bind_s(ld, userDN.toAscii().data(), password.toAscii().data());
    struct berval cred;
    cred.bv_len = password.size();
    cred.bv_val = password.toAscii().data();

    struct berval *serverCred = NULL;

    error = ldap_sasl_bind_s(ld, userDN.toAscii().data(), LDAP_SASL_SIMPLE , &cred, NULL, NULL, &serverCred);
    if (error != LDAP_SUCCESS ) {
        QMessageBox::critical(0, "openLDAP", QString("Failed to bind (error: %1)").arg(error));
        return false;
    }
    qDebug() << "Bound to" << connectionString << error;

    this->connected = true;
    return true;
}

void LdapHelper::disconnect() {
    ldap_unbind_ext(ld, NULL, NULL);
}

QString LdapHelper::getUserFromIButton(QString id) {
    LDAPMessage *res;

    char *attr[2];
    attr[0] = (char *)"uid";
    attr[1] = NULL;

    struct timeval time;
    time.tv_sec = 2;
    time.tv_usec = 0;

    qDebug() << "Checking" << id;

    int error = ldap_search_ext_s(ld, "ou=Users,dc=csh,dc=rit,dc=edu", LDAP_SCOPE_SUBTREE,
                                  QString("(&(objectClass=ibuttonUser)(ibutton=%1))").arg(id).toAscii().data(),
                                  attr, 0, NULL, NULL, &time, 100, &res);


    if (error != LDAP_SUCCESS) {
        if (error == LDAP_FILTER_ERROR) {
            errString = "Invalid iButton ID";
            return "";
        }

        QMessageBox::critical(0, "openLDAP", QString("Failed to query iButton (error: %1)").arg(error));
        return "";
    }

    if (ldap_count_entries(ld, res) > 0) {
        res = ldap_first_entry(ld, res);

        struct berval *value = *ldap_get_values_len(ld, res, "uid");

        QString user = value->bv_val;
        //ldap_value_free_len(&value);
        qDebug() << "Found" << user;

        return user;
    }

    qDebug() << "Invalid iButton ID";

    return "";
}

QString LdapHelper::getLastError() {
    return errString;
}